#include <errno.h>
#include <getopt.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <sqlite3.h>

#include "io.h"
#include "resource.h"
#include "shell.h"
#include "utils.h"

global_opts_t global_opts;

// clang-format off
int cmd_init (int, char *[]);
int cmd_add  (int, char *[]);
int cmd_rm   (int, char *[]);
int cmd_find (int, char *[]);
int cmd_show (int, char *[]);
int cmd_sync (int, char *[]);
// clang-format on

int main(int argc, char *argv[]) {
  opterr = 0; // Disable getopt's own error messages
  parse_global_opts(&global_opts, argc, argv);

  if (optind >= argc)
    ERROR_USAGE_EXIT("Error processing args: Expect a subcommand\n", stderr);

  char *subcmd = argv[optind];
  int (*cmd_func)(int, char *[]);
#define CMP(str) (strcmp(subcmd, str) == 0)
  if CMP ("init")
    cmd_func = cmd_init;
  else if CMP ("add")
    cmd_func = cmd_add;
  else if CMP ("rm")
    cmd_func = cmd_rm;
  else if CMP ("find")
    cmd_func = cmd_find;
  else if CMP ("show")
    cmd_func = cmd_show;
  else if CMP ("sync")
    cmd_func = cmd_sync;
#undef CMP
  else
    ERROR_USAGE_EXIT("Error processing args: Unknown subcommand '%s'\n",
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
    char response = getchar();
    if (response != 'y' && response != 'Y') {
      puts("Aborting initialization.");
      exit(EXIT_SUCCESS);
    }
  }

  if (global_opts.verbose)
    puts("Resolving directory path...");
  char *path = realpath(opts.dir, NULL);
  if (!path)
    ERROR_EXIT("Error resolving directory path '%s': %s", opts.dir,
               strerror(errno));

  if (global_opts.verbose)
    printf("Resolved directory path: %s\n", path);

  if (global_opts.verbose)
    puts("Constructing database folder path...");
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

  if (global_opts.verbose)
    puts("Creating database directory...");
  if (mkdir(data_path, 0755) != 0)
    PERROR_EXIT("Error creating database directory");

  if (global_opts.verbose)
    puts("Constructing database file path...");
  char *db_path = get_db_path(data_path);
  if (global_opts.verbose)
    printf("Constructed database file path: %s\n", db_path);

  if (global_opts.verbose)
    puts("Initializing database...");
  sqlite3 *db = init_db(db_path, global_opts.verbose);

  printf("Tag database initialized in '%s'\n", data_path);

  if (global_opts.verbose)
    puts("Closing database connection...");
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

  char *real_path = realpath(opts.file, NULL);
  if (!real_path)
    ERROR_EXIT("Error resolving file path '%s': %s\n", opts.file,
               strerror(errno));
  char *relative_path = get_relative_path(r.data_root, real_path);
  if (global_opts.verbose)
    printf("Resolved real path: %s\nRelative path: %s\n", real_path,
           relative_path);

  long long file_id =
      add_file(r.db, real_path, relative_path, global_opts.verbose);
  add_tags(r.db, file_id, opts.tags, opts.tags_count, global_opts.verbose);

  if (opts.tags_count > 1)
    printf("Added file '%s' with %zu tags\n", opts.file, opts.tags_count);
  else
    printf("Added file '%s' with tag '%s'\n", opts.file, opts.tags[0]);

  free(relative_path);
  free(real_path);
  cleanup_resources(&r);

  return 0;
}

int cmd_rm(int argc, char *argv[]) {
  rm_opts_t opts = {0};
  parse_rm_opts(&opts, argc, argv);

  return 0;
}

int cmd_find(int argc, char *argv[]) {
  find_opts_t opts = {};
  parse_find_opts(&opts, argc, argv);

  return 0;
}

int cmd_show(int argc, char *argv[]) {
  show_opts_t opts = {};
  parse_show_opts(&opts, argc, argv);

  return 0;
}

int cmd_sync(int argc, char *argv[]) {
  sync_opts_t opts = {};
  parse_sync_opts(&opts, argc, argv);

  return 0;
}
