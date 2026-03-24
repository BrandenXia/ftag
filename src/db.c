#include "db.h"

#include <stdio.h>
#include <stdlib.h>

#include <sqlite3.h>

#include "hash.h"
#include "utils.h"

#define ERROR_SQL_EXIT(msg)                                                    \
  do {                                                                         \
    fprintf(stderr, msg ": %s\n", sqlite3_errmsg(db));                         \
    sqlite3_close(db);                                                         \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define CHECK_BIND_ERR(param)                                                  \
  if (err != SQLITE_OK)                                                        \
    ERROR_SQL_EXIT("Error binding " param);

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

#define SQL_CREATE_FILES_TABLE                                                 \
  "CREATE TABLE IF NOT EXISTS files (\n" SQL_FILES_SCHEMA ");"
#define SQL_CREATE_TAGS_TABLE                                                  \
  "CREATE TABLE IF NOT EXISTS tags (\n" SQL_TAGS_SCHEMA ");"
#define SQL_CREATE_FILE_TAGS_TABLE                                             \
  "CREATE TABLE IF NOT EXISTS file_tags (\n" SQL_FILE_TAGS_SCHEMA ");"
#define SQL_CREATE_SCHEMA                                                      \
  SQL_CREATE_FILES_TABLE SQL_CREATE_TAGS_TABLE SQL_CREATE_FILE_TAGS_TABLE

sqlite3 *init_db(const char *path, bool verbose) {
  sqlite3 *db;
  int err;

  if (verbose)
    printf("Opening database at path: %s\n", path);
  err = sqlite3_open(path, &db);
  if (err != SQLITE_OK)
    ERROR_SQL_EXIT("Error opening database");
  if (verbose)
    puts("Database opened successfully.");

  if (verbose)
    puts("Creating database schema...");
  char *errmsg;
  err = sqlite3_exec(db, SQL_CREATE_SCHEMA, NULL, NULL, &errmsg);
  if (err != SQLITE_OK) {
    fprintf(stderr, "Error creating database schema: %s\n", errmsg);
    sqlite3_free(errmsg);
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }
  if (verbose)
    puts("Database schema prepared successfully.");

  return db;
}

#define SQL_INSERT_FILE                                                        \
  "INSERT INTO files (path, is_dir, size, mtime, hash) VALUES (?, ?, ?, ?, "   \
  "?) ON CONFLICT(path) DO UPDATE SET path=excluded.path RETURNING id;"

long long add_file(sqlite3 *db, const char *real_path,
                   const char *relative_path, bool verbose) {
  if (verbose)
    printf("Adding file to database: real_path='%s', relative_path='%s'\n",
           real_path, relative_path);

  int err;

  if (verbose)
    puts("Retrieving file info...");
  file_info_t info;
  get_file_info(real_path, &info);
  uint8_t hash[32];
  if (info.is_dir)
    err = hash_dir(real_path, hash);
  else
    err = hash_file(real_path, hash);
  if (err != 0) {
    sqlite3_close(db);
    ERROR_EXIT("Error hashing file: %s\n", real_path);
  }

  if (verbose) {
    printf("Inserting file into database with info: is_dir=%d, size=%lld, "
           "mtime=%lld\n",
           info.is_dir, info.size, info.mtime);
    printf("File hash: ");
    print_hash(hash);
  }

  sqlite3_stmt *stmt;
  SQL_PREPARE(stmt, SQL_INSERT_FILE);
  SQL_BIND(text, stmt, 1, relative_path, "path");
  SQL_BIND_NUM(int, stmt, 2, info.is_dir, "is_dir");
  SQL_BIND_NUM(int64, stmt, 3, info.size, "size");
  SQL_BIND_NUM(int64, stmt, 4, info.mtime, "mtime");
  SQL_BIND_BLOB(stmt, 5, hash, sizeof(hash), "hash");

  SQL_STEP(stmt, != SQLITE_ROW);
  long long file_id = sqlite3_column_int64(stmt, 0);
  SQL_FINALIZE(stmt);

  return file_id;
}

#define SQL_INSERT_TAG                                                         \
  "INSERT INTO tags (name) VALUES (?) ON CONFLICT(name) DO UPDATE SET "        \
  "name=excluded.name RETURNING id;"

#define SQL_INSERT_FILE_TAG                                                    \
  "INSERT OR IGNORE INTO file_tags (file_id, tag_id) VALUES (?, ?);"

void add_tags(sqlite3 *db, long long file_id, const char **tags,
              size_t tags_count, bool verbose) {
  sqlite3_stmt *stmt;

  for (size_t i = 0; i < tags_count; i++) {
    const char *tag = tags[i];

    if (verbose)
      printf("Preparing tag '%s' in database...\n", tag);
    SQL_PREPARE(stmt, SQL_INSERT_TAG);
    SQL_BIND(text, stmt, 1, tag, "tag name");
    SQL_STEP(stmt, != SQLITE_ROW);
    long long tag_id = sqlite3_column_int64(stmt, 0);
    SQL_FINALIZE(stmt);

    if (verbose)
      printf("Adding tag '%s' (id: %lld) to file with id %lld\n", tag, tag_id,
             file_id);
    SQL_PREPARE(stmt, SQL_INSERT_FILE_TAG);
    SQL_BIND_NUM(int64, stmt, 1, file_id, "file_id");
    SQL_BIND_NUM(int64, stmt, 2, tag_id, "tag_id");
    SQL_STEP(stmt);
    SQL_FINALIZE(stmt);
  }
}
