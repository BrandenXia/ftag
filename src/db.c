#include "db.h"

#include <stdio.h>
#include <stdlib.h>

#include <sqlite3.h>

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
  if (err != SQLITE_OK) {
    fprintf(stderr, "Error opening database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }
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
