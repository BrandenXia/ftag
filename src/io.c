#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "hash.h"
#include "resource.h"
#include "utils.h"

sqlite3 *init_db(const char *path, bool verbose) {
  if (verbose)
    printf("Opening database at path: %s\n", path);
  sqlite3 *db = open_db(path);
  if (verbose)
    puts("Database opened successfully.");

  if (verbose)
    puts("Creating database schema...");
  setup_db(db);
  if (verbose)
    puts("Database schema prepared successfully.");

  return db;
}

void setup_resources(resources_t *r, bool verbose) {
  char *cwd = realpath(".", NULL);
  if (!cwd)
    PERROR_EXIT("Error resolving current working directory");

  size_t cwd_len = strlen(cwd);
  r->data_root = malloc(cwd_len + 1);
  r->data_path = malloc(cwd_len + sizeof(DATA_DIRNAME) + 1);
  if (!r->data_path)
    PERROR_EXIT("Memory allocation failed for data_path");
  find_data_dir(cwd, r->data_root, r->data_path);
  free(cwd);

  if (verbose)
    printf("Data path: %s\n", r->data_path);
  r->db_path = get_db_path(r->data_path);
  r->db = init_db(r->db_path, verbose);
}

void cleanup_resources(resources_t *r) {
  sqlite3_close(r->db);
  free(r->db_path);
  free(r->data_root);
  free(r->data_path);
}

long long add_or_get_file(sqlite3 *db, const char *real_path,
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

  return add_or_get_file_id(db, relative_path, info.is_dir, info.size,
                            info.mtime, hash);
}

void add_tags(sqlite3 *db, long long file_id, const char **tags,
              size_t tags_count, bool verbose) {
  begin_transaction(db);
  for (size_t i = 0; i < tags_count; i++) {
    const char *tag = tags[i];

    if (verbose)
      printf("Preparing tag '%s' in database...\n", tag);
    long long tag_id = add_or_get_tag_id(db, tag);

    if (verbose)
      printf("Adding tag '%s' (id: %lld) to file with id %lld\n", tag, tag_id,
             file_id);
    add_file_tag(db, file_id, tag_id);
  }
  commit_transaction(db);
}

void remove_tags(sqlite3 *db, long long file_id, const char **tags,
                 size_t tags_count, bool force, bool verbose) {
  begin_transaction(db);
  for (size_t i = 0; i < tags_count; i++) {
    const char *tag = tags[i];

    if (verbose)
      printf("Preparing tag '%s' in database...\n", tag);
    long long tag_id = add_or_get_tag_id(db, tag);

    if (verbose)
      printf("Removing tag '%s' (id: %lld) from file with id %lld\n", tag,
             tag_id, file_id);
    remove_file_tag(db, file_id, tag_id);

    int changes = sqlite3_changes(db);
    if (changes == 0 && !force)
      printf("Warning: Tag '%s' was not associated with file id %lld, nothing "
             "removed\n",
             tag, file_id);
  }
  commit_transaction(db);
}

void print_file_path(const char *path, void *user_data) {
  const char *relative_to = user_data;
  if (strcmp(relative_to, ".") == 0)
    puts(path);
  else {
    char *relative_path = get_relative_path(relative_to, path);
    puts(relative_path);
    free(relative_path);
  }
}

void query_files(sqlite3 *db, const char **tags, size_t tags_count,
                 enum tag_match_mode match_mode, const char *relative_to) {
  int found_count = query_files_by_tags(
      db, tags, tags_count, match_mode,
      (query_context_t){print_file_path, (void *)relative_to});

  if (found_count == 0)
    printf("No files found matching the specified tags.\n");
}
