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
#define SQL_ENABLE_FOREIGN_KEYS "PRAGMA foreign_keys = ON;"
#define SQL_CREATE_FILE_TAGS_INDEX                                             \
  "CREATE INDEX IF NOT EXISTS idx_file_tags_tag_id ON file_tags(tag_id, "      \
  "file_id);"
#define SQL_SETUP_DB                                                           \
  SQL_ENABLE_FOREIGN_KEYS SQL_CREATE_SCHEMA SQL_CREATE_FILE_TAGS_INDEX

void setup_db(sqlite3 *db) {
  char *errmsg;
  int err = sqlite3_exec(db, SQL_SETUP_DB, NULL, NULL, &errmsg);
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
                             int64_t size, int64_t mtime, uint8_t *hash) {
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

#define SQL_DELETE_ALL_FILE_TAGS "DELETE FROM file_tags WHERE file_id = ?;"

void remove_all_tags(sqlite3 *db, long long file_id) {
  sqlite3_stmt *stmt;
  SQL_PREPARE(stmt, SQL_DELETE_ALL_FILE_TAGS);
  SQL_BIND_NUM(int64, stmt, 1, file_id, "file_id");
  SQL_STEP(stmt);
  SQL_FINALIZE(stmt);
}

#define SQL_QUERY_FILES_BY_TAGS_AND                                            \
  "SELECT files.* FROM files JOIN file_tags ON files.id = file_tags.file_id "  \
  "JOIN tags ON file_tags.tag_id = tags.id WHERE tags.name IN (%s) GROUP BY "  \
  "files.id HAVING COUNT(tags.id) = %d;"

#define SQL_QUERY_FILES_BY_TAGS_OR                                             \
  "SELECT DISTINCT files.* FROM files JOIN file_tags ON files.id = "           \
  "file_tags.file_id JOIN tags ON file_tags.tag_id = tags.id WHERE tags.name " \
  "IN (%s);"

#define SQL_QUERY_FILES_BY_TAGS_RELEVANCE                                      \
  "SELECT files.*, COUNT(tags.id) as match_count FROM files JOIN file_tags "   \
  "ON files.id = file_tags.file_id JOIN tags ON file_tags.tag_id = tags.id "   \
  "WHERE tags.name IN (%s) GROUP BY files.id ORDER BY match_count DESC;"

int query_files_by_tags(sqlite3 *db, const char **tags, size_t tags_count,
                        enum tag_match_mode mode, query_files_ctx_t ctx) {
  const char *template;
  switch (mode) {
  case TAG_MATCH_RELEVANCE:
    template = SQL_QUERY_FILES_BY_TAGS_RELEVANCE;
    break;
  case TAG_MATCH_OR:
    template = SQL_QUERY_FILES_BY_TAGS_OR;
    break;
  case TAG_MATCH_AND:
    template = SQL_QUERY_FILES_BY_TAGS_AND;
    break;
  }

  size_t placeholders_len = tags_count * 2;      // "?, ?, ..."
  char *placeholders = malloc(placeholders_len); // "?, ?, ..."
  placeholders[0] = '?';
  placeholders[placeholders_len - 1] = '\0';
  for (size_t i = 1; i < tags_count; i++) {
    placeholders[i * 2 - 1] = ',';
    placeholders[i * 2] = '?';
  }

  size_t query_len =
      (size_t)snprintf(NULL, 0, template, placeholders, tags_count);
  char *query = malloc(query_len + 1);
  snprintf(query, query_len + 1, template, placeholders, tags_count);

  sqlite3_stmt *stmt;
  SQL_PREPARE(stmt, query);
  for (size_t i = 0; i < tags_count; i++)
    SQL_BIND(text, stmt, (int)i + 1, tags[i], "tag name");

  int count = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *path = (const char *)sqlite3_column_text(stmt, 1);
    bool is_dir = sqlite3_column_int(stmt, 2);
    ctx.callback(path, is_dir, ctx.user_data);
    count++;
  }

  SQL_FINALIZE(stmt);

  return count;
}

#define SQL_FIND_TAGS_BY_FILE                                                  \
  "SELECT tags.name FROM tags JOIN file_tags ON tags.id = file_tags.tag_id "   \
  "WHERE file_tags.file_id = ?;"

int query_tags_by_file(sqlite3 *db, long long file_id, find_tags_ctx_t ctx) {
  sqlite3_stmt *stmt;
  SQL_PREPARE(stmt, SQL_FIND_TAGS_BY_FILE);
  SQL_BIND_NUM(int64, stmt, 1, file_id, "file_id");

  int count = 0;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char *tag = (const char *)sqlite3_column_text(stmt, 0);
    ctx.callback(tag, ctx.user_data);
    count++;
  }

  SQL_FINALIZE(stmt);

  return count;
}
