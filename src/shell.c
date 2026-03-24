#include "shell.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "utils.h"

// static inline char *shift_args(int *argc, char ***argv) {
//   char *first_arg = NULL;
//   if (*argc > 0) {
//     first_arg = (*argv)[0];
//     (*argv)++;
//     (*argc)--;
//   }
//   return first_arg;
// }

#define UNKOWN_OPT_MSG "Error processing arguments: Unknown option '-%c'\n"

// -------------------------GLOBAL -------------------------
// clang-format off
static struct option global_long_opts[] = {
  {"verbose", no_argument, NULL, 'v'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0}
};
// clang-format on
void parse_global_opts(global_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "+vh", global_long_opts, NULL)) != -1)
    // clang-format off
    switch (opt) {
    case 'v': opts->verbose = true; break;
    case 'h': fputs(USAGE_STR_GLOBAL, stdout); exit(EXIT_SUCCESS);
    // clang-format on
    case '?':
      ERROR_USAGE_EXIT(UNKOWN_OPT_MSG, optopt);
    }
}

// --------------------------INIT --------------------------
// clang-format off
static struct option init_long_opts[] = {
  {"help", no_argument, NULL, 'h'},
  {"force", no_argument, NULL, 'f'},
  {NULL, 0, NULL, 0}
};
// clang-format on
void parse_init_opts(init_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "hf", init_long_opts, NULL)) != -1)
    // clang-format off
    switch (opt) {
    case 'h': fputs(USAGE_STR_INIT, stdout); exit(EXIT_SUCCESS);
    case 'f': opts->force = true; break;
    // clang-format on
    case '?':
      ERROR_USAGE_EXIT(UNKOWN_OPT_MSG, optopt);
    }

  if (optind < argc)
    opts->dir = argv[optind];
  else
    opts->dir = ".";
}

// --------------------------ADD --------------------------
// clang-format off
static struct option add_long_opts[] = {
  {"help", no_argument, NULL, 'h'},
  {"force", no_argument, NULL, 'f'},
  {NULL, 0, NULL, 0}
};
// clang-format on
void parse_add_opts(add_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "hf", add_long_opts, NULL)) != -1)
    // clang-format off
    switch (opt) {
    case 'h': fputs(USAGE_STR_ADD, stdout); exit(EXIT_SUCCESS);
    case 'f': opts->force = true; break;
    // clang-format on
    case '?':
      ERROR_USAGE_EXIT(UNKOWN_OPT_MSG, optopt);
    }
}

// -------------------------REMOVE -------------------------
// clang-format off
static struct option rm_long_opts[] = {
  {"help", no_argument, NULL, 'h'},
  {"all", no_argument, NULL, 'a'},
  {"force", no_argument, NULL, 'f'},
  {NULL, 0, NULL, 0}
};
// clang-format on
void parse_rm_opts(rm_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "haf", rm_long_opts, NULL)) != -1)
    // clang-format off
    switch (opt) {
    case 'h': fputs(USAGE_STR_RM, stdout); exit(EXIT_SUCCESS);
    case 'a': opts->all = true; break;
    case 'f': opts->force = true; break;
    // clang-format on
    case '?':
      ERROR_USAGE_EXIT(UNKOWN_OPT_MSG, optopt);
    }
}

// --------------------------FIND --------------------------
// clang-format off
static struct option find_long_opts[] = {
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0}
};
// clang-format on
void parse_find_opts(find_opts_t *, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "h", find_long_opts, NULL)) != -1)
    // clang-format off
    switch (opt) {
    case 'h': fputs(USAGE_STR_FIND, stdout); exit(EXIT_SUCCESS);
    // clang-format on
    case '?':
      ERROR_USAGE_EXIT(UNKOWN_OPT_MSG, optopt);
    }
}

// --------------------------SHOW --------------------------
// clang-format off
static struct option show_long_opts[] = {
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0}
};
// clang-format on
void parse_show_opts(show_opts_t *, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "h", show_long_opts, NULL)) != -1)
    // clang-format off
    switch (opt) {
    case 'h': fputs(USAGE_STR_SHOW, stdout); exit(EXIT_SUCCESS);
    // clang-format on
    case '?':
      ERROR_USAGE_EXIT(UNKOWN_OPT_MSG, optopt);
    }
}

// --------------------------SYNC --------------------------
// clang-format off
static struct option sync_long_opts[] = {
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0}
};
// clang-format on
void parse_sync_opts(sync_opts_t *, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "h", sync_long_opts, NULL)) != -1)
    // clang-format off
    switch (opt) {
    case 'h': fputs(USAGE_STR_SYNC, stdout); exit(EXIT_SUCCESS);
    // clang-format on
    case '?':
      ERROR_USAGE_EXIT(UNKOWN_OPT_MSG, optopt);
    }
}
