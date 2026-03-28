#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <sqlite3.h>

#include "db.h"
#include "io.h"
#include "resource.h"
#include "shell.h"
#include "utils.h"

global_opts_t global_opts;

// clang-format off
int cmd_init    (int, char *[]);
int cmd_add     (int, char *[]);
int cmd_rm      (int, char *[]);
int cmd_copy    (int, char *[]);
int cmd_rename  (int, char *[]);
int cmd_show    (int, char *[]);
int cmd_sync    (int, char *[]);
int cmd_cleanup (int, char *[]);
int cmd_query   (int, char *[], bool is_find_cmd);
// clang-format on

int main(int argc, char *argv[]) {
  opterr = 0; // Disable getopt's own error messages
  parse_global_opts(&global_opts, argc, argv);

  if (optind >= argc)
    ERROR_USAGE_EXIT(USAGE_STR_GLOBAL,
                     "Error processing args: Expect a subcommand\n");

  char *subcmd = argv[optind];
  int (*cmd_func)(int, char *[]);
#define CMP(str) (strcmp(subcmd, str) == 0)
  if CMP ("init")
    cmd_func = cmd_init;
  else if CMP ("add")
    cmd_func = cmd_add;
  else if CMP ("rm")
    cmd_func = cmd_rm;
  else if CMP ("copy")
    cmd_func = cmd_copy;
  else if CMP ("rename")
    cmd_func = cmd_rename;
  else if CMP ("show")
    cmd_func = cmd_show;
  else if CMP ("sync")
    cmd_func = cmd_sync;
  else if CMP ("cleanup")
    cmd_func = cmd_cleanup;
  else if CMP ("query") {
    int shift = optind;
    optind = 1; // Reset optind for subcommand parsing
    return cmd_query(argc - shift, argv + shift, false);
  } else if CMP ("find") {
    int shift = optind;
    optind = 1; // Reset optind for subcommand parsing
    return cmd_query(argc - shift, argv + shift, true);
  }
#undef CMP
  else
    ERROR_USAGE_EXIT(USAGE_STR_GLOBAL,
                     "Error processing args: Unknown subcommand '%s'\n",
                     subcmd);

  int shift = optind;
  optind = 1; // Reset optind for subcommand parsing
  return cmd_func(argc - shift, argv + shift);
}

int cmd_init(int argc, char *argv[]) {
  init_opts_t opts = {0};
  parse_init_opts(&opts, argc, argv);

  if (global_opts.verbose)
    printf("Initializing tag database in directory '%s' with force=%s\n",
           opts.dir, opts.force ? "true" : "false");

  if (opts.force) {
    puts("Warning: Force option is enabled. Existing tag database will be "
         "overwritten if it exists.");
    printf("Continue? [y/N] ");
    char response = (char)getchar();
    if (response != 'y' && response != 'Y') {
      puts("Aborting initialization.");
      exit(EXIT_SUCCESS);
    }
  }

  if (global_opts.verbose) puts("Resolving directory path...");
  char *path = realpath(opts.dir, NULL);
  if (!path)
    ERROR_EXIT("Error resolving directory path '%s': %s", opts.dir,
               strerror(errno));

  if (global_opts.verbose) printf("Resolved directory path: %s\n", path);

  if (global_opts.verbose) puts("Constructing database folder path...");
  char *data_path = get_data_path(path);
  free(path);
  if (global_opts.verbose)
    printf("Constructed database folder path: %s\n", data_path);

  if (global_opts.verbose)
    puts("Checking the current state of the database directory...");
  struct stat st;
  if (stat(data_path, &st) == 0) {
    if (!opts.force) {
      fprintf(stderr, "Error creating database: %s already exists\n",
              data_path);
      ERROR_EXIT("Please either remove the existing database directory or "
                 "run command with --force option to overwrite it.\n");
    }
    if (rmrf(data_path) != 0)
      PERROR_EXIT("Error removing existing database directory");

    puts("Existing database directory removed.");
  } else if (errno != ENOENT)
    PERROR_EXIT("Error checking directory");

  if (global_opts.verbose) puts("Creating database directory...");
  if (mkdir(data_path, 0755) != 0)
    PERROR_EXIT("Error creating database directory");

  if (global_opts.verbose) puts("Constructing database file path...");
  char *db_path = get_db_path(data_path);
  if (global_opts.verbose)
    printf("Constructed database file path: %s\n", db_path);

  if (global_opts.verbose) puts("Initializing database...");
  sqlite3 *db = init_db(db_path, global_opts.verbose);

  printf("Tag database initialized in '%s'\n", data_path);

  if (global_opts.verbose) puts("Closing database connection...");
  sqlite3_close(db);
  free(db_path);
  free(data_path);

  return 0;
}

int cmd_add(int argc, char *argv[]) {
  add_opts_t opts = {0};
  parse_add_opts(&opts, argc, argv);
  resources_t r;
  setup_resources(&r, global_opts.verbose);

  auto resolved =
      resolve_file_path(opts.file, r.data_root, global_opts.verbose);

  long long file_id =
      add_or_get_file(r.db, resolved.real_path, resolved.relative_path,
                      opts.strict, global_opts.verbose);
  add_tags(r.db, file_id, opts.tags, opts.tags_count, global_opts.verbose);

  if (opts.tags_count > 1)
    printf("Added file '%s' with %zu tags\n", opts.file, opts.tags_count);
  else
    printf("Added file '%s' with tag '%s'\n", opts.file, opts.tags[0]);

  free(resolved.relative_path);
  free(resolved.real_path);
  cleanup_resources(&r);

  return 0;
}

int cmd_rm(int argc, char *argv[]) {
  rm_opts_t opts = {0};
  parse_rm_opts(&opts, argc, argv);
  resources_t r;
  setup_resources(&r, global_opts.verbose);

  auto resolved =
      resolve_file_path(opts.file, r.data_root, global_opts.verbose);

  long long file_id =
      add_or_get_file(r.db, resolved.real_path, resolved.relative_path,
                      opts.strict, global_opts.verbose);
  if (opts.all)
    remove_all_tags(r.db, file_id);
  else
    remove_tags(r.db, file_id, opts.tags, opts.tags_count, opts.force,
                global_opts.verbose);

  if (opts.all)
    printf("Removed all tags from file '%s'\n", opts.file);
  else {
    if (opts.tags_count > 1)
      printf("Removed %zu tags from file '%s'\n", opts.tags_count, opts.file);
    else
      printf("Removed tag '%s' from file '%s'\n", opts.tags[0], opts.file);
  }

  free(resolved.relative_path);
  free(resolved.real_path);
  cleanup_resources(&r);

  return 0;
}

int cmd_copy(int argc, char *argv[]) {
  copy_opts_t opts = {0};
  parse_copy_opts(&opts, argc, argv);
  resources_t r;
  setup_resources(&r, global_opts.verbose);

  auto resolved_src =
      resolve_file_path(opts.src, r.data_root, global_opts.verbose);
  auto resolved_dst =
      resolve_file_path(opts.dst, r.data_root, global_opts.verbose);

  auto src_id =
      add_or_get_file(r.db, resolved_src.real_path, resolved_src.relative_path,
                      opts.strict, global_opts.verbose);
  auto dst_id =
      add_or_get_file(r.db, resolved_dst.real_path, resolved_dst.relative_path,
                      opts.strict, global_opts.verbose);

  copy_file_tags(r.db, src_id, dst_id);

  int copied_count = sqlite3_changes(r.db);
  printf("Updated file '%s' with %d tags from file '%s'\n", opts.dst,
         copied_count, opts.src);

  free(resolved_dst.real_path);
  free(resolved_dst.relative_path);
  free(resolved_src.real_path);
  free(resolved_src.relative_path);
  cleanup_resources(&r);

  return 0;
}

int cmd_rename(int argc, char *argv[]) {
  rename_opts_t opts = {0};
  parse_rename_opts(&opts, argc, argv);
  resources_t r;
  setup_resources(&r, global_opts.verbose);

  rename_tag(r.db, opts.old_tag, opts.new_tag, opts.force, global_opts.verbose);

  cleanup_resources(&r);

  return 0;
}

int cmd_query(int argc, char *argv[], bool is_find_cmd) {
  query_opts_t opts = {0};
  if (is_find_cmd) opts.dir = ".";
  parse_query_opts(&opts, argc, argv);
  resources_t r;
  setup_resources(&r, global_opts.verbose);

  char *cwd = realpath(".", NULL);
  char *relative_cwd = get_relative_path(r.data_root, cwd);
  free(cwd);

  if (global_opts.verbose) printf("Relative CWD for query: %s\n", relative_cwd);

  char *dir = NULL;
  if (opts.dir) {
    char *dir_absolute = realpath(opts.dir, NULL);
    if (!dir_absolute)
      ERROR_EXIT("Error resolving directory path '%s': %s\n", opts.dir,
                 strerror(errno));
    dir = get_relative_path(r.data_root, dir_absolute);
    free(dir_absolute);
  }

  union relevance_or_regex u;
  if (opts.match_mode == TAG_MATCH_RELEVANCE) {
    u.relevance.tags_count = opts.relevance.tags_count;
    u.relevance.tags = opts.relevance.tags;
  } else
    u.regex = opts.regex;
  query_files(r.db, opts.match_mode, u, relative_cwd, dir, opts.type,
              global_opts.verbose);

  free(relative_cwd);
  free(dir);
  cleanup_resources(&r);

  return 0;
}

int cmd_show(int argc, char *argv[]) {
  show_opts_t opts = {};
  parse_show_opts(&opts, argc, argv);
  resources_t r;
  setup_resources(&r, global_opts.verbose);

  auto resolved =
      resolve_file_path(opts.file, r.data_root, global_opts.verbose);

  long long file_id =
      add_or_get_file(r.db, resolved.real_path, resolved.relative_path,
                      opts.strict, global_opts.verbose);
  show_tags(r.db, file_id);

  free(resolved.relative_path);
  free(resolved.real_path);
  cleanup_resources(&r);

  return 0;
}

int cmd_sync(int argc, char *argv[]) {
  sync_opts_t opts = {};
  parse_sync_opts(&opts, argc, argv);

  resources_t r;
  setup_resources(&r, global_opts.verbose);

  sync_tags(r.db, r.data_root, opts.dry_run, opts.deep, opts.yes,
            global_opts.verbose);

  cleanup_resources(&r);

  return 0;
}

int cmd_cleanup(int argc, char *argv[]) {
  cleanup_opts_t opts = {};
  parse_cleanup_opts(&opts, argc, argv);

  resources_t r;
  setup_resources(&r, global_opts.verbose);

  int update_count;
  cleanup_tags(r.db);
  update_count = sqlite3_changes(r.db);
  printf("Removed %d orphaned tags\n", update_count);
  cleanup_files(r.db);
  update_count = sqlite3_changes(r.db);
  printf("Removed %d orphaned files\n", update_count);
  vacuum_db(r.db);

  cleanup_resources(&r);

  return 0;
}
