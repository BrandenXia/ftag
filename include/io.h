#ifndef TAG_IO_H
#define TAG_IO_H

#include <stdbool.h>
#include <stddef.h>

#include <sqlite3.h>

#include "db.h"
#include "resource.h"
#include "shell.h"

sqlite3 *init_db(const char *path, bool verbose);

void setup_resources(resources_t *r, bool verbose);
void cleanup_resources(resources_t *r);

long long add_or_get_file(sqlite3 *db, const char *real_path,
                          const char *relative_path, bool verbose);

void add_tags(sqlite3 *db, long long file_id, const char **tags,
              size_t tags_count, bool verbose);

void remove_tags(sqlite3 *db, long long file_id, const char **tags,
                 size_t tags_count, bool force, bool verbose);

void query_files(sqlite3 *db, const char **tags, size_t tags_count,
                 enum tag_match_mode match_mode, const char *relative_to,
                 const char *dir, enum query_type type, bool verbose);

void show_tags(sqlite3 *db, long long file_id);

#endif
