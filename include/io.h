#ifndef TAG_IO_H
#define TAG_IO_H

#include <stdbool.h>
#include <stddef.h>

#include <sqlite3.h>

#include "resource.h"
#include "shell.h"

sqlite3 *init_db(const char *path, bool verbose);

void setup_resources(resources_t *r, bool verbose);
void cleanup_resources(resources_t *r);

// This function allocates memory for the real_path and relative_path in the
// returned struct, the caller is responsible for freeing them
struct resolve_file_path_result {
  char *real_path;
  char *relative_path;
} resolve_file_path(const char *file, const char *data_root, bool verbose);

long long add_or_get_file(sqlite3 *db, const char *real_path,
                          const char *relative_path, bool strict, bool verbose);

void add_tags(sqlite3 *db, long long file_id, const char **tags,
              size_t tags_count, bool verbose);

void remove_tags(sqlite3 *db, long long file_id, const char **tags,
                 size_t tags_count, bool force, bool verbose);

void rename_tag(sqlite3 *db, const char *old_name, const char *new_name,
                bool force, bool verbose);

union relevance_or_regex {
  struct {
    size_t tags_count;
    const char **tags;
  } relevance;
  const char *regex;
};
void query_files(sqlite3 *db, enum tag_match_mode match_mode,
                 union relevance_or_regex u, const char *relative_to,
                 const char *dir, enum query_file_type type, bool verbose);

void show_tags(sqlite3 *db, long long file_id);

void list_files(sqlite3 *db, const char *relative_to, const char *dir);

void list_tags(sqlite3 *db);

void show_stats(sqlite3 *db, size_t top_tags_limit);

void sync_tags(sqlite3 *db, const char *data_root, bool dry_run, bool deep,
               bool confirm, bool verbose);

#endif
