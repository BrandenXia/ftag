#include "io.h"

#include <ctype.h>
#include <errno.h>
#include <fts.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__linux__) && !defined(PATH_MAX)
#include <linux/limits.h>
#endif

#include "stb/stb_ds.h"

#include "db.h"
#include "hash.h"
#include "resource.h"
#include "utils.h"

sqlite3 *init_db(const char *path, bool verbose) {
  if (verbose) printf("Opening database at path: %s\n", path);
  sqlite3 *db = open_db(path);
  if (verbose) puts("Database opened successfully.");

  if (verbose) puts("Creating database schema...");
  setup_db(db);
  if (verbose) puts("Database schema prepared successfully.");

  return db;
}

void setup_resources(resources_t *r, bool verbose) {
  char *cwd = realpath(".", NULL);
  if (!cwd) PERROR_EXIT("Error resolving current working directory");

  size_t cwd_len = strlen(cwd);
  r->data_root = malloc(cwd_len + 1);
  r->data_path = malloc(cwd_len + sizeof(DATA_DIRNAME) + 1);
  if (!r->data_path) PERROR_EXIT("Memory allocation failed for data_path");
  find_data_dir(cwd, r->data_root, r->data_path);
  free(cwd);

  if (verbose) printf("Data path: %s\n", r->data_path);
  r->db_path = get_db_path(r->data_path);
  r->db = init_db(r->db_path, verbose);
}

void cleanup_resources(resources_t *r) {
  sqlite3_close(r->db);
  free(r->db_path);
  free(r->data_root);
  free(r->data_path);
}

struct resolve_file_path_result
resolve_file_path(const char *file, const char *data_root, bool verbose) {
  char *real_path = realpath(file, NULL);
  if (!real_path)
    ERROR_EXIT("Error resolving file path '%s': %s\n", file, strerror(errno));
  char *relative_path = get_relative_path(data_root, real_path);
  if (verbose)
    printf("Resolved real path: %s\nRelative path: %s\n", real_path,
           relative_path);

  return (struct resolve_file_path_result){real_path, relative_path};
}

struct check_file_info_ctx {
  bool strict;
  bool *match;
  long long *file_id;
  file_info_t *info;
};

void check_file_info(sqlite3_stmt *stmt, void *user_data) {
  struct check_file_info_ctx *ctx = user_data;
  file_info_t *info = ctx->info;

  const char *path = (const char *)sqlite3_column_text(stmt, 1);
  bool is_dir = sqlite3_column_int(stmt, 2);
  int64_t size = sqlite3_column_int64(stmt, 3);
  int64_t mtime = sqlite3_column_int64(stmt, 4);

  if (is_dir != info->is_dir || size != info->size || mtime != info->mtime) {
    if (ctx->strict) {
      fprintf(stderr,
              "Error: file '%s' already exists in the database with different "
              "info (is_dir=%d, size=%" PRId64 ", mtime=%" PRId64 ")\n",
              path, is_dir, size, mtime);
      exit(EXIT_FAILURE);
    }

    printf("Warning: file '%s' already exists in the database with different "
           "info (is_dir=%d, size=%" PRId64 ", mtime=%" PRId64
           "), updating info\n",
           path, is_dir, size, mtime);
    printf("You can use '--strict' option to prevent this and treat it as an "
           "error instead\n");
    *ctx->match = false;
    return;
  }

  *ctx->match = true;
  *ctx->file_id = sqlite3_column_int64(stmt, 0);
}

long long add_or_get_file(sqlite3 *db, const char *real_path,
                          const char *relative_path, bool strict,
                          bool verbose) {
  if (verbose)
    printf("Checking if file '%s' is already in the database...\n",
           relative_path);

  int err;
  if (verbose) puts("Retrieving file info...");
  file_info_t info;
  get_file_info(real_path, &info);

  bool match;
  long long file_id;
  struct check_file_info_ctx ctx = {
    .strict = strict,
    .match = &match,
    .file_id = &file_id,
    .info = &info,
  };
  query_file_by_path(
      db, relative_path,
      (db_query_ctx_t){.callback = check_file_info, .user_data = &ctx});
  if (match) return file_id;

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
    printf("Inserting file into database with info: is_dir=%d, size=%" PRId64
           ", "
           "mtime=%" PRId64 "\n",
           info.is_dir, info.size, info.mtime);
    printf("File hash: ");
    print_hash(hash);
  }

  return add_or_update_file(db, relative_path, info.is_dir, info.size,
                            info.mtime, hash);
}

void add_tags(sqlite3 *db, long long file_id, const char **tags,
              size_t tags_count, bool verbose) {
  begin_transaction(db);
  for (size_t i = 0; i < tags_count; i++) {
    const char *tag = tags[i];

    if (verbose) printf("Preparing tag '%s' in database...\n", tag);
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

    if (verbose) printf("Preparing tag '%s' in database...\n", tag);
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

void rename_tag(sqlite3 *db, const char *old_name, const char *new_name,
                bool force, bool verbose) {
  if (verbose) printf("Renaming tag '%s' to '%s'...\n", old_name, new_name);

  bool success = update_tag_check_conflict(db, old_name, new_name);
  if (!success) {
    if (!force)
      ERROR_EXIT(
          "Error: Tag '%s' already exists, cannot rename '%s' to '%s'. Use "
          "--force option to overwrite the existing tag.\n",
          new_name, old_name, new_name);

    if (verbose)
      printf(
          "Tag '%s' already exists, merging tags '%s' into '%s' and removing "
          "tag '%s'\n",
          new_name, old_name, new_name, old_name);
    long long old_tag_id = add_or_get_tag_id(db, old_name);
    long long new_tag_id = add_or_get_tag_id(db, new_name);
    merge_tags(db, old_tag_id, new_tag_id);
  }

  if (sqlite3_changes(db) == 0)
    printf("Warning: Tag '%s' does not exist, nothing renamed\n", old_name);
  else
    printf("Renamed tag '%s' to '%s'\n", old_name, new_name);
}

struct print_file_ctx {
  const char *relative_to;
  const char *dir;
  enum query_file_type type;
  bool verbose;
};

void print_file_path(sqlite3_stmt *stmt, void *user_data) {
  const char *path = (const char *)sqlite3_column_text(stmt, 1);
  bool is_dir = sqlite3_column_int(stmt, 2);

  struct print_file_ctx *ctx = user_data;
  const char *relative_to = ctx->relative_to;

  if (ctx->dir && strcmp(ctx->dir, ".") != 0) {
    // check if path is under the specified directory
    // if not, skip it
    if (strncmp(path, ctx->dir, strlen(ctx->dir)) != 0 ||
        (path[strlen(ctx->dir)] != '/' && path[strlen(ctx->dir)] != '\0'))
      return;
  }

  if (ctx->type == QUERY_TYPE_FILE && is_dir) return;
  if (ctx->type == QUERY_TYPE_DIR && !is_dir) return;

  if (strcmp(relative_to, ".") == 0)
    puts(path);
  else {
    char *relative_path = get_relative_path(relative_to, path);
    puts(relative_path);
    free(relative_path);
  }
}

void query_files(sqlite3 *db, enum tag_match_mode match_mode,
                 union relevance_or_regex u, const char *relative_to,
                 const char *dir, enum query_file_type type, bool verbose) {
  struct print_file_ctx ctx = {
    .relative_to = relative_to,
    .dir = dir,
    .type = type,
    .verbose = verbose,
  };
  db_query_ctx_t query_ctx = {.callback = print_file_path, .user_data = &ctx};

  int found_count;
  switch (match_mode) {
  case TAG_MATCH_RELEVANCE:
    found_count = query_file_tags_relevance(db, u.relevance.tags,
                                            u.relevance.tags_count, query_ctx);
    break;
  case TAG_MATCH_REGEX:
    const char *regex = u.regex; // in regex mode, treats the first tag as regex
    found_count = query_file_tags_regex(db, regex, query_ctx);
    break;
  }

  if (found_count == 0) printf("No files found matching the specified tags.\n");
}

void print_tag(sqlite3_stmt *stmt, void *) {
  const char *tag = (const char *)sqlite3_column_text(stmt, 0);
  puts(tag);
}

void show_tags(sqlite3 *db, long long file_id) {
  int found_count =
      query_tags_by_file(db, file_id, (db_query_ctx_t){print_tag, NULL});

  if (found_count == 0) printf("No tags found for the specified file.\n");
}

void list_files(sqlite3 *db, const char *relative_to, const char *dir) {
  struct print_file_ctx ctx = {
    .relative_to = relative_to,
    .dir = dir,
    .type = QUERY_TYPE_BOTH,
    .verbose = false,
  };
  query_all_files(db, (db_query_ctx_t){print_file_path, &ctx});
}

void list_tags(sqlite3 *db) {
  query_all_tags_name(db, (db_query_ctx_t){print_tag, NULL});
}

struct show_stats_top_tag_ctx {
  int *index;
  int total_tagged_files;
};

void show_stats_top_tag_cb(sqlite3_stmt *stmt, void *user_data) {
  struct show_stats_top_tag_ctx *ctx = user_data;
  const char *tag = (const char *)sqlite3_column_text(stmt, 0);
  int file_count = sqlite3_column_int(stmt, 1);
  printf("%d. %s (%d files, %.1f%% of tagged files)\n", *ctx->index, tag,
         file_count, (double)file_count / ctx->total_tagged_files * 100);
  (*ctx->index)++;
}

void show_stats(sqlite3 *db, size_t top_tags_limit) {
  int tag_count = count_tags(db);
  int file_count = count_files(db);
  int file_tag_count = count_file_tags(db);

  printf(
      "%d existing tags, %d tagged files, %d tag-file associations on record\n",
      tag_count, file_count, file_tag_count);

  printf("Top %zu tags:\n", top_tags_limit);
  int index = 1;
  struct show_stats_top_tag_ctx ctx = {
    .index = &index, .total_tagged_files = file_count};
  query_top_tags(db, top_tags_limit,
                 (db_query_ctx_t){show_stats_top_tag_cb, &ctx});
}

struct missing_file_entry {
  long long id;
  char *path;
  bool is_dir;
  int64_t size;
  int64_t mtime;
  uint8_t hash[32];
};

struct sync_tags_ctx {
  sqlite3 *db;
  const char *data_root;
  char *path_buf;
  size_t *updated_count;
  struct missing_file_entry **missing_files;
  bool dry_run;
  bool deep;
  bool verbose;
};

void sync_tags_iter_cb(sqlite3_stmt *stmt, void *user_data) {
  struct sync_tags_ctx *ctx = user_data;

  long long file_id = sqlite3_column_int64(stmt, 0);
  int64_t size = sqlite3_column_int64(stmt, 3);
  int64_t mtime = sqlite3_column_int64(stmt, 4);
  bool is_dir = sqlite3_column_int(stmt, 2);
  uint8_t *hash_db = (uint8_t *)sqlite3_column_blob(stmt, 5);

  file_info_t info;
  const char *path = (const char *)sqlite3_column_text(stmt, 1);
  char *full_path = concat_paths(ctx->data_root, path, ctx->path_buf, PATH_MAX);
  if (access(full_path, F_OK) != 0) {
    printf("File '%s' is in the database but does not exist on disk\n", path);
    char *missing_path = strdup(path);
    struct missing_file_entry entry = {
      .id = file_id,
      .path = missing_path,
      .is_dir = is_dir,
      .size = size,
      .mtime = mtime,
    };
    memcpy(entry.hash, hash_db, 32);
    arrput(*ctx->missing_files, entry);
    return;
  }

  get_file_info(full_path, &info);

  if (is_dir == info.is_dir && size == info.size && mtime == info.mtime) {
    if (!ctx->deep) {
      if (ctx->verbose)
        printf("File '%s' info matches database, skipping\n", path);
      return;
    }

    printf("File '%s' info matches database, but performing deep check due to "
           "--deep option\n",
           path);
  }

  int err;
  uint8_t hash[32];
  if (info.is_dir)
    err = hash_dir(full_path, hash);
  else
    err = hash_file(full_path, hash);
  if (err != 0) {
    sqlite3_close(ctx->db);
    ERROR_EXIT("Error hashing file: %s\n", full_path);
  }

  if (ctx->deep) {
    if (memcmp(hash, hash_db, 32) == 0) {
      if (ctx->verbose)
        printf("File '%s' hash matches database, skipping update\n", path);
      return;
    }
  }

  printf("File '%s' already exists in the database with different "
         "info (is_dir=%d, size=%" PRId64 ", mtime=%" PRId64
         "), updating info\n",
         path, is_dir, size, mtime);

  if (ctx->verbose) {
    printf("Updating file '%s' info in database to: is_dir=%d, size=%" PRId64
           ", "
           "mtime=%" PRId64 "\n",
           path, info.is_dir, info.size, info.mtime);
    printf("File hash: ");
    print_hash(hash);
  }

  if (!ctx->dry_run)
    add_or_update_file(ctx->db, path, info.is_dir, info.size, info.mtime, hash);
  (*ctx->updated_count)++;
}

struct file_exact_idx_entry {
  struct file_exact_idx_key {
    int64_t size;
    int64_t mtime;
  } key;
  char **value; // path
};

struct file_fallback_idx_entry {
  int64_t key;  // size
  char **value; // path
};

struct dir_hash_idx_entry {
  struct dir_hash_idx_key {
    uint8_t hash[32];
  } key;
  char **value; // path
};

void init_dir_hash_idx(const char *data_root,
                       struct dir_hash_idx_entry **dir_hash_idx) {
  FTS *fts = fts_open((char *[]){(char *)data_root, NULL},
                      FTS_NOCHDIR | FTS_PHYSICAL, NULL);
  if (!fts)
    ERROR_EXIT("Error opening file tree for syncing: %s\n", strerror(errno));

  FTSENT *ent;
  while ((ent = fts_read(fts)) != NULL) {
    if (ent->fts_info != FTS_DP) continue;

    struct dir_hash_idx_entry idx_entry = {.value = NULL};

    char *path = strdup(ent->fts_path);
    if (hash_dir(ent->fts_path, idx_entry.key.hash) != 0) {
      fts_close(fts);
      ERROR_EXIT("Error hashing directory: %s\n", ent->fts_path);
    }

    arrput(idx_entry.value, path);
    hmputs(*dir_hash_idx, idx_entry);
  }
}

char *resolve_missing_file(struct missing_file_entry *missing_file_entry,
                           struct file_exact_idx_entry *file_exact_idx,
                           struct file_fallback_idx_entry *file_fallback_idx) {
  uint8_t hash_buf[32];

  struct file_exact_idx_key exact_key = {
    .size = missing_file_entry->size,
    .mtime = missing_file_entry->mtime,
  };
  char **candidates = hmget(file_exact_idx, exact_key);
  if (arrlen(candidates) == 1)
    return candidates[0];
  else if (arrlen(candidates) > 0)
    for (int i = 0; i < arrlen(candidates); i++) {
      if (hash_file(candidates[i], hash_buf) != 0)
        ERROR_EXIT("Error hashing file: %s\n", candidates[i]);
      if (memcmp(hash_buf, missing_file_entry->hash, 32) == 0)
        return candidates[i];
    }

  candidates = hmget(file_fallback_idx, missing_file_entry->size);
  if (arrlen(candidates) > 0)
    for (int i = 0; i < arrlen(candidates); i++) {
      if (hash_file(candidates[i], hash_buf) != 0)
        ERROR_EXIT("Error hashing file: %s\n", candidates[i]);
      if (memcmp(hash_buf, missing_file_entry->hash, 32) == 0)
        return candidates[i];
    }

  // unable to resolve missing file
  return NULL;
}

char *resolve_missing_dir(struct missing_file_entry *missing_file_entry,
                          struct dir_hash_idx_entry *dir_hash_idx,
                          char ***dir_candidates) {
  struct dir_hash_idx_key dir_key;
  memcpy(dir_key.hash, missing_file_entry->hash, 32);
  char **candidates = hmget(dir_hash_idx, dir_key);
  if (arrlen(candidates) == 1) return candidates[0];

  *dir_candidates = candidates;
  return NULL;
}

void sync_tags(sqlite3 *db, const char *data_root, bool dry_run, bool deep,
               bool confirm, bool verbose) {
  size_t updated_count = 0;
  struct missing_file_entry *missing_files = NULL;
  char path_buf[PATH_MAX];
  struct sync_tags_ctx ctx = {
    .db = db,
    .data_root = data_root,
    .path_buf = path_buf,
    .updated_count = &updated_count,
    .missing_files = &missing_files, // triple pointer because stb_ds modifies
                                     // the pointer when resizing
    .dry_run = dry_run,
    .deep = deep,
    .verbose = verbose,
  };
  iter_files(
      db, (db_query_ctx_t){.callback = sync_tags_iter_cb, .user_data = &ctx});

  if (arrlen(missing_files) == 0) {
    printf("All files in the database exist on disk, no missing files to "
           "sync.\n");
    return;
  }

  printf("Found %zu files in the database that are missing on disk:\n",
         arrlen(missing_files));

  struct file_exact_idx_entry *file_exact_idx = NULL;
  struct file_fallback_idx_entry *file_fallback_idx = NULL;
  hmdefault(file_exact_idx, NULL);
  hmdefault(file_fallback_idx, NULL);

  struct dir_hash_idx_entry *dir_hash_idx = NULL;

  char *paths[] = {(char *)data_root, NULL};
  FTS *fts = fts_open(paths, FTS_NOCHDIR | FTS_PHYSICAL, NULL);
  if (!fts)
    ERROR_EXIT("Error opening file tree for syncing: %s\n", strerror(errno));

  FTSENT *ent;
  while ((ent = fts_read(fts)) != NULL) {
    if (ent->fts_info != FTS_F) continue;

    char *path = strdup(ent->fts_path);
    int64_t size = ent->fts_statp->st_size;
    int64_t mtime = ent->fts_statp->st_mtime;

    struct file_exact_idx_key idx_key = {
      .size = size,
      .mtime = mtime,
    };
    char **exact_idx_paths = hmget(file_exact_idx, idx_key);
    arrput(exact_idx_paths, path);
    hmput(file_exact_idx, idx_key, exact_idx_paths);

    char **fallback_idx_paths = hmget(file_fallback_idx, size);
    arrput(fallback_idx_paths, path);
    hmput(file_fallback_idx, size, fallback_idx_paths);

    break;
  }
  fts_close(fts);

  int missing_resolved_count = 0;
  int deleted_count = 0;

  for (int i = 0; i < arrlen(missing_files); i++) {
    struct missing_file_entry missing_file = missing_files[i];

    char *resolved_path;
    char **dir_candidates = NULL;
    if (missing_file.is_dir) {
      if (!dir_hash_idx) init_dir_hash_idx(data_root, &dir_hash_idx);
      resolved_path =
          resolve_missing_dir(&missing_file, dir_hash_idx, &dir_candidates);
    } else
      resolved_path = resolve_missing_file(&missing_file, file_exact_idx,
                                           file_fallback_idx);

    char *relative_path;
    if (resolved_path != NULL) {
      relative_path = get_relative_path(data_root, resolved_path);
      printf("Resolved missing file '%s' to existing path '%s'\n",
             missing_file.path, relative_path);

      if (!confirm) {
        printf("Confirm update? ([Y]es/[N]o/[D]elete entry): ");
        char choice;
        while (true) {
          choice = (char)getchar();
          if (isupper(choice)) choice = (char)tolower((char)choice);
          if (choice == 'y' || choice == 'n' || choice == 'd') break;
          printf("Invalid choice, please enter y, n, or d: ");
        }
        if (choice == 'n')
          continue;
        else if (choice == 'd') {
          if (!dry_run) delete_file(ctx.db, missing_file.id);
          deleted_count++;
          continue;
        }
        // if choice is 'y', proceed with update
      }
    } else {
      // unable to resolve missing file, ask user
      printf("Unable to automatically resolve missing file '%s', which has "
             "size %" PRId64 " and mtime %" PRId64 "\n",
             missing_file.path, missing_file.size, missing_file.mtime);

      int idx = 1;
      if (!missing_file.is_dir) {
        struct file_exact_idx_key exact_key = {
          .size = missing_file.size,
          .mtime = missing_file.mtime,
        };
        char **exact_candidates = hmget(file_exact_idx, exact_key);
        char **fallback_candidates =
            hmget(file_fallback_idx, missing_file.size);

        if (exact_candidates != NULL) {
          printf("Exact candidates (matching size and mtime):\n");
          for (int j = 0; j < arrlen(exact_candidates); j++)
            printf("%d. %s\n", idx++, exact_candidates[j]);
        }

        if (fallback_candidates != NULL) {
          printf("Fallback candidates (matching size):\n");
          for (int j = 0; j < arrlen(fallback_candidates); j++)
            printf("%d. %s\n", idx++, fallback_candidates[j]);
        }
      } else if (dir_candidates != NULL) {
        printf("Directory candidates (matching hash):\n");
        for (int j = 0; j < arrlen(dir_candidates); j++)
          printf("%d. %s\n", idx++, dir_candidates[j]);
      }

      if (idx != 1) {
        printf("Choose a candidate to resolve the missing file\n");
        puts("Index of candidate to choose, or 0 to skip:");
        int choice;
        (void)scanf("%d", &choice);
        while (choice < 0 || choice >= idx) {
          printf("Invalid choice, please enter a number between 0 and %d: ",
                 idx - 1);
          (void)scanf("%d", &choice);
        }

        if (choice != 0) {
          if (missing_file.is_dir) {
            resolved_path = dir_candidates[choice - 1];
          } else {
            struct file_exact_idx_key exact_key = {
              .size = missing_file.size,
              .mtime = missing_file.mtime,
            };
            char **exact_candidates = hmget(file_exact_idx, exact_key);
            char **fallback_candidates =
                hmget(file_fallback_idx, missing_file.size);
            if (choice - 1 < arrlen(exact_candidates))
              resolved_path = exact_candidates[choice - 1];
            else if (choice - 1 - arrlen(exact_candidates) <
                     arrlen(fallback_candidates))
              resolved_path =
                  fallback_candidates[choice - 1 - arrlen(exact_candidates)];
            else
              // should never happen because of the choice validation
              ERROR_EXIT("Error resolving missing file: invalid choice index");
          }
          relative_path = get_relative_path(data_root, resolved_path);
          goto resolved;
        }
      }

      printf("No candidate available / chosen, enter new path, delete, or skip "
             "this file\n");
      char choice;
      while (true) {
        printf("Enter path to [R]esolve, [D]elete entry, or [S]kip: ");
        choice = (char)getchar();
        if (isupper(choice)) choice = (char)tolower((char)choice);
        if (choice == 'r' || choice == 'd' || choice == 's') break;
        printf("Invalid choice, please enter r, d, or s: ");
      }
      if (choice == 'd') {
        if (!dry_run) delete_file(ctx.db, missing_file.id);
        deleted_count++;
        continue;
      } else if (choice == 's')
        continue;
      // the rest is if choice is 'r'

      puts("No candidate available / chosen, manually return new path to "
           "resolve missing file, or leave empty to skip:");
      puts("Note: the path should be relative to the data root directory");
      printf("New path: ");
      char tmp_buf[PATH_MAX];
      (void)scanf("%s", tmp_buf);
      concat_paths(data_root, tmp_buf, path_buf, PATH_MAX);
      resolved_path = path_buf;
      relative_path = get_relative_path(data_root, resolved_path);
    }

  resolved:
    file_info_t info;
    get_file_info(resolved_path, &info);
    uint8_t hash[32];
    int err;
    if (info.is_dir)
      err = hash_dir(resolved_path, hash);
    else
      err = hash_file(resolved_path, hash);
    if (err != 0) ERROR_EXIT("Error hashing file: %s\n", resolved_path);
    if (!dry_run)
      update_file(ctx.db, missing_file.id, relative_path, info.is_dir,
                  info.size, info.mtime, hash);

    printf("Updated file '%s' in database to point to resolved path '%s'\n",
           missing_file.path, relative_path);
    missing_resolved_count++;

    free(relative_path);
  }

  printf("Sync complete. Updated info for %zu files, resolved %d missing "
         "files, deleted %d missing file entries.\n",
         updated_count, missing_resolved_count, deleted_count);

  for (int i = 0; i < arrlen(missing_files); i++)
    free(missing_files[i].path);
  arrfree(missing_files);

  for (int i = 0; i < hmlen(file_exact_idx); i++) {
    char **file_paths = file_exact_idx[i].value;
    for (int j = 0; j < arrlen(file_paths); j++)
      free(file_paths[j]);
    arrfree(file_paths);
  }
  hmfree(file_exact_idx);

  for (int i = 0; i < hmlen(file_fallback_idx); i++) {
    char **file_paths = file_fallback_idx[i].value;
    // don't free the file paths here because they are shared with the exact idx
    arrfree(file_paths);
  }
  hmfree(file_fallback_idx);

  for (int i = 0; i < hmlen(dir_hash_idx); i++) {
    char **dir_paths = dir_hash_idx[i].value;
    for (int j = 0; j < arrlen(dir_paths); j++)
      free(dir_paths[j]);
    arrfree(dir_paths);
  }
  hmfree(dir_hash_idx);
}
