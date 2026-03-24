#include "db.h"

#include <stdio.h>
#include <stdlib.h>

#include <sqlite3.h>

#define ERROR_SQL_EXIT(msg)                                                    \
  do {                                                                         \
    fprintf(stderr, msg ": %s\n", sqlite3_errmsg(db));                         \
    sqlite3_close(db);                                                         \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define CHECK_BIND_ERR(param)                                                  \
  if (err != SQLITE_OK)                                                        \
    ERROR_SQL_EXIT("Error binding " param);

#define SQL_EXEC(sql)                                                          \
  do {                                                                         \
    char *errmsg;                                                              \
    int err = sqlite3_exec(db, (sql), NULL, NULL, &errmsg);                    \
    if (err != SQLITE_OK) {                                                    \
      fprintf(stderr, "Error executing SQL statement: %s\n", errmsg);          \
      sqlite3_free(errmsg);                                                    \
      sqlite3_close(db);                                                       \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

#define SQL_PREPARE(stmt, sql)                                                 \
  do {                                                                         \
    int err = sqlite3_prepare_v2(db, (sql), -1, &(stmt), NULL);                \
    if (err != SQLITE_OK)                                                      \
      ERROR_SQL_EXIT("Error preparing SQL statement");                         \
  } while (0)

#define SQL_BIND(type, stmt, idx, value, name)                                 \
  do {                                                                         \
    int err =                                                                  \
        sqlite3_bind_##type((stmt), (idx), (value), -1, SQLITE_TRANSIENT);     \
    if (err != SQLITE_OK)                                                      \
      ERROR_SQL_EXIT("Error binding " name " parameter at index " #idx);       \
  } while (0)

#define SQL_BIND_NUM(type, stmt, idx, value, name)                             \
  do {                                                                         \
    int err = sqlite3_bind_##type((stmt), (idx), (value));                     \
    if (err != SQLITE_OK)                                                      \
      ERROR_SQL_EXIT("Error binding " #name " parameter at index " #idx);      \
  } while (0)

#define SQL_BIND_BLOB(stmt, idx, value, size, name)                            \
  do {                                                                         \
    int err =                                                                  \
        sqlite3_bind_blob((stmt), (idx), (value), (size), SQLITE_TRANSIENT);   \
    if (err != SQLITE_OK)                                                      \
      ERROR_SQL_EXIT("Error binding " name " blob parameter at index " #idx);  \
  } while (0)

#define _SQL_STEP(stmt, cond)                                                  \
  do {                                                                         \
    int err = sqlite3_step(stmt);                                              \
    if (err cond)                                                              \
      ERROR_SQL_EXIT("Error executing SQL statement");                         \
  } while (0)
#define _SQL_STEP_DEFAULT(stmt) _SQL_STEP(stmt, != SQLITE_DONE)

#define NTH_ARG(_1, _2, COND, ...) COND
#define CHOOSE_SQL_STEP(...) NTH_ARG(__VA_ARGS__, _SQL_STEP, _SQL_STEP_DEFAULT)
#define SQL_STEP(...) CHOOSE_SQL_STEP(__VA_ARGS__)(__VA_ARGS__)

#define SQL_FINALIZE(stmt)                                                     \
  do {                                                                         \
    int err = sqlite3_finalize(stmt);                                          \
    if (err != SQLITE_OK)                                                      \
      ERROR_SQL_EXIT("Error finalizing SQL statement");                        \
  } while (0)

void begin_transaction(sqlite3 *db) { SQL_EXEC("BEGIN TRANSACTION;"); }
void commit_transaction(sqlite3 *db) { SQL_EXEC("COMMIT;"); }

#define SQL_FILES_SCHEMA                                                       \
  "id INTEGER PRIMARY KEY,\n"                                                  \
  ""                                                                           \
  "path TEXT UNIQUE NOT NULL,\n"                                               \
  "is_dir BOOLEAN NOT NULL,\n"                                                 \
  "" /* file identities */                                                     \
  "size INTEGER NOT NULL,\n"                                                   \
  "mtime INTEGER NOT NULL,\n"                                                  \
  "hash TEXT NOT NULL\n"

#define SQL_TAGS_SCHEMA                                                        \
  "id INTEGER PRIMARY KEY,\n"                                                  \
  "name TEXT UNIQUE NOT NULL\n"

#define SQL_FILE_TAGS_SCHEMA                                                   \
  "file_id INTEGER NOT NULL,\n"                                                \
  "tag_id INTEGER NOT NULL,\n"                                                 \
  "PRIMARY KEY (file_id, tag_id),\n"                                           \
  "FOREIGN KEY (file_id) REFERENCES files(id) ON DELETE CASCADE,\n"            \
  "FOREIGN KEY (tag_id) REFERENCES tags(id) ON DELETE CASCADE"

sqlite3 *open_db(const char *path) {
  sqlite3 *db;
  int err = sqlite3_open(path, &db);
  if (err != SQLITE_OK)
    ERROR_SQL_EXIT("Error opening database");
  return db;
}

#define SQL_CREATE_FILES_TABLE                                                 \
  "CREATE TABLE IF NOT EXISTS files (\n" SQL_FILES_SCHEMA ");"
#define SQL_CREATE_TAGS_TABLE                                                  \
  "CREATE TABLE IF NOT EXISTS tags (\n" SQL_TAGS_SCHEMA ");"
#define SQL_CREATE_FILE_TAGS_TABLE                                             \
  "CREATE TABLE IF NOT EXISTS file_tags (\n" SQL_FILE_TAGS_SCHEMA ");"
#define SQL_CREATE_SCHEMA                                                      \
  SQL_CREATE_FILES_TABLE SQL_CREATE_TAGS_TABLE SQL_CREATE_FILE_TAGS_TABLE

void setup_db(sqlite3 *db) {
  char *errmsg;
  int err = sqlite3_exec(db, SQL_CREATE_SCHEMA, NULL, NULL, &errmsg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "Error creating database schema: %s\n", errmsg);
    sqlite3_free(errmsg);
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }
}

#define SQL_INSERT_FILE                                                        \
  "INSERT INTO files (path, is_dir, size, mtime, hash) VALUES (?, ?, ?, ?, "   \
  "?) ON CONFLICT(path) DO UPDATE SET path=excluded.path RETURNING id;"

long long add_or_get_file_id(sqlite3 *db, const char *path, bool is_dir,
                             uint64_t size, uint64_t mtime, uint8_t *hash) {
  sqlite3_stmt *stmt;
  SQL_PREPARE(stmt, SQL_INSERT_FILE);
  SQL_BIND(text, stmt, 1, path, "path");
  SQL_BIND_NUM(int, stmt, 2, is_dir, "is_dir");
  SQL_BIND_NUM(int64, stmt, 3, size, "size");
  SQL_BIND_NUM(int64, stmt, 4, mtime, "mtime");
  SQL_BIND_BLOB(stmt, 5, hash, 32, "hash");

  SQL_STEP(stmt, != SQLITE_ROW);
  long long file_id = sqlite3_column_int64(stmt, 0);
  SQL_FINALIZE(stmt);

  return file_id;
}

#define SQL_INSERT_TAG                                                         \
  "INSERT INTO tags (name) VALUES (?) ON CONFLICT(name) DO UPDATE SET "        \
  "name=excluded.name RETURNING id;"

long long add_or_get_tag_id(sqlite3 *db, const char *name) {
  sqlite3_stmt *stmt;
  SQL_PREPARE(stmt, SQL_INSERT_TAG);
  SQL_BIND(text, stmt, 1, name, "tag name");

  SQL_STEP(stmt, != SQLITE_ROW);
  long long tag_id = sqlite3_column_int64(stmt, 0);
  SQL_FINALIZE(stmt);

  return tag_id;
}

#define SQL_INSERT_FILE_TAG                                                    \
  "INSERT OR IGNORE INTO file_tags (file_id, tag_id) VALUES (?, ?);"

void add_file_tag(sqlite3 *db, long long file_id, long long tag_id) {
  sqlite3_stmt *stmt;
  SQL_PREPARE(stmt, SQL_INSERT_FILE_TAG);
  SQL_BIND_NUM(int64, stmt, 1, file_id, "file_id");
  SQL_BIND_NUM(int64, stmt, 2, tag_id, "tag_id");
  SQL_STEP(stmt);
  SQL_FINALIZE(stmt);
}

#define SQL_DELETE_FILE_TAG                                                    \
  "DELETE FROM file_tags WHERE file_id = ? AND tag_id = ?;"
void remove_file_tag(sqlite3 *db, long long file_id, long long tag_id) {
  sqlite3_stmt *stmt;
  SQL_PREPARE(stmt, SQL_DELETE_FILE_TAG);
  SQL_BIND_NUM(int64, stmt, 1, file_id, "file_id");
  SQL_BIND_NUM(int64, stmt, 2, tag_id, "tag_id");
  SQL_STEP(stmt);
  SQL_FINALIZE(stmt);
}
