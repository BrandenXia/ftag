#ifndef FTAG_DB_H
#define FTAG_DB_H

#include <stdbool.h>
#include <stddef.h>

#include <sqlite3.h>

#define DB_FILENAME "db.sqlite3"

sqlite3 *init_db(const char *path, bool verbose);

// adds a file to the database if it doesn't exist, and returns its id
long long add_file(sqlite3 *db, const char *real_path,
                   const char *relative_path, bool verbose);

void add_tags(sqlite3 *db, long long file_id, const char **tags,
              size_t tags_count, bool verbose);

#endif
