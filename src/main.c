#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "db.h"
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
    PERROR_EXIT("Error resolving directory path");

  if (global_opts.verbose)
    printf("Resolved directory path: %s\n", path);

  if (global_opts.verbose)
    puts("Constructing database folder path...");
  size_t path_len = strlen(path);
  size_t folder_path_len = path_len + sizeof(DB_DIRNAME) + 1; // +1 for '/'
  char folder_path[folder_path_len];
  snprintf(folder_path, sizeof(folder_path), "%s/%s", path, DB_DIRNAME);
  if (global_opts.verbose)
    printf("Constructed database folder path: %s\n", folder_path);

  if (global_opts.verbose)
    puts("Checking the current state of the database directory...");
  struct stat st;
  if (stat(folder_path, &st) == 0) {
    if (!opts.force) {
      fprintf(stderr, "Error creating database: %s already exists\n",
              folder_path);
      ERROR_EXIT("Please either remove the existing database directory or "
                 "run command with --force option to overwrite it.\n");
    }
    if (rmrf(folder_path) != 0)
      PERROR_EXIT("Error removing existing database directory");

    puts("Existing database directory removed.");
  } else if (errno != ENOENT)
    PERROR_EXIT("Error checking directory");

  if (global_opts.verbose)
    puts("Creating database directory...");
  if (mkdir(folder_path, 0755) != 0)
    PERROR_EXIT("Error creating database directory");

  if (global_opts.verbose)
    puts("Constructing database file path...");
  size_t db_path_len = folder_path_len + sizeof(DB_FILENAME) + 1; // +1 for '/'
  char db_path[db_path_len];
  snprintf(db_path, sizeof(db_path), "%s/%s", folder_path, DB_FILENAME);
  if (global_opts.verbose)
    printf("Constructed database file path: %s\n", db_path);

  if (global_opts.verbose)
    puts("Initializing database...");
  init_db(db_path, global_opts.verbose);

  printf("Tag database initialized in '%s'\n", folder_path);

  return 0;
}

int cmd_add(int argc, char *argv[]) {
  add_opts_t opts = {0};
  parse_add_opts(&opts, argc, argv);

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
