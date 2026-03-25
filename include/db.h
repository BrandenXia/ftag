#ifndef FTAG_DB_H
#define FTAG_DB_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <sqlite3.h>

#define DB_FILENAME "db.sqlite3"

sqlite3_stmt *prepare_stmt(sqlite3 *db, const char *sql);
void begin_transaction(sqlite3 *db);
void commit_transaction(sqlite3 *db);

sqlite3 *open_db(const char *path);

void setup_db(sqlite3 *db);

long long add_or_get_file_id(sqlite3 *db, const char *path, bool is_dir,
                             int64_t size, int64_t mtime, uint8_t *hash);

long long add_or_get_tag_id(sqlite3 *db, const char *name);

void add_file_tag(sqlite3 *db, long long file_id, long long tag_id);

void remove_file_tag(sqlite3 *db, long long file_id, long long tag_id);

void remove_all_tags(sqlite3 *db, long long file_id);

enum tag_match_mode {
  TAG_MATCH_OR,
  TAG_MATCH_AND,
  TAG_MATCH_RELEVANCE,
};

typedef void (*file_path_cb_t)(const char *path, bool is_dir, void *user_data);
typedef struct {
  file_path_cb_t callback;
  void *user_data;
} query_context_t;

// Returns the number of files found
int query_files_by_tags(sqlite3 *db, const char **tags, size_t tags_count,
                        enum tag_match_mode mode, query_context_t ctx);

typedef void (*tag_cb_t)(const char *tag, void *user_data);
typedef struct {
  tag_cb_t callback;
  void *user_data;
} show_tags_context_t;

int find_tags_by_file(sqlite3 *db, long long file_id, show_tags_context_t ctx);

#endif
