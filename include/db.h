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

typedef void (*db_query_cb_t)(sqlite3_stmt *stmt, void *user_data);
typedef struct {
  db_query_cb_t callback;
  void *user_data;
} db_query_ctx_t;

sqlite3 *open_db(const char *path);

void setup_db(sqlite3 *db);

long long add_or_update_file(sqlite3 *db, const char *path, bool is_dir,
                             int64_t size, int64_t mtime, uint8_t *hash);

void update_file(sqlite3 *db, long long file_id, const char *path, bool is_dir,
                 int64_t size, int64_t mtime, uint8_t *hash);

void delete_file(sqlite3 *db, long long file_id);

void query_file_by_path(sqlite3 *db, const char *path, db_query_ctx_t ctx);

long long add_or_get_tag_id(sqlite3 *db, const char *name);

// Returns false on conflict
bool update_tag_check_conflict(sqlite3 *db, const char *old_name,
                               const char *new_name);

void merge_tags(sqlite3 *db, long long src_tag_id, long long dst_tag_id);

void add_file_tag(sqlite3 *db, long long file_id, long long tag_id);

void remove_file_tag(sqlite3 *db, long long file_id, long long tag_id);

void remove_all_tags(sqlite3 *db, long long file_id);

void copy_file_tags(sqlite3 *db, long long src_file_id, long long dst_file_id);

// Returns the number of files found
int query_file_tags_relevance(sqlite3 *db, const char **tags, size_t tags_count,
                              db_query_ctx_t ctx);

int query_file_tags_regex(sqlite3 *db, const char *regex, db_query_ctx_t ctx);

// Returns the number of files found
int query_tags_by_file(sqlite3 *db, long long file_id, db_query_ctx_t ctx);

void iter_files(sqlite3 *db, db_query_ctx_t ctx);

#endif
