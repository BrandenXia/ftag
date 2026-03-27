#include "shell.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utils.h"

#define UNKOWN_OPT_MSG "Error processing arguments: Unknown option '-%c'\n"

// -------------------------GLOBAL -------------------------
static struct option global_long_opts[] = {
  {"verbose", no_argument, NULL, 'v'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0},
};
void parse_global_opts(global_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "+vh", global_long_opts, NULL)) != -1)
    switch (opt) {
    case 'v': opts->verbose = true; break;
    case 'h': fputs(USAGE_STR_GLOBAL, stdout); exit(EXIT_SUCCESS);
    case '?': ERROR_USAGE_EXIT(USAGE_STR_GLOBAL, UNKOWN_OPT_MSG, optopt);
    }
}

// --------------------------INIT --------------------------
static struct option init_long_opts[] = {
  {"help", no_argument, NULL, 'h'},
  {"force", no_argument, NULL, 'f'},
  {NULL, 0, NULL, 0},
};
void parse_init_opts(init_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "hf", init_long_opts, NULL)) != -1)
    switch (opt) {
    case 'h': fputs(USAGE_STR_INIT, stdout); exit(EXIT_SUCCESS);
    case 'f': opts->force = true; break;
    case '?': ERROR_USAGE_EXIT(USAGE_STR_INIT, UNKOWN_OPT_MSG, optopt);
    }

  if (optind < argc)
    opts->dir = argv[optind];
  else
    opts->dir = ".";

  if (argc > optind + 1)
    ERROR_USAGE_EXIT(USAGE_STR_INIT,
                     "Error processing args: Expect at most a directory\n");
}

// --------------------------ADD --------------------------
static struct option add_long_opts[] = {
  {"strict", no_argument, NULL, 's'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0},
};
void parse_add_opts(add_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "sh", add_long_opts, NULL)) != -1)
    switch (opt) {
    case 's': opts->strict = true; break;
    case 'h': fputs(USAGE_STR_ADD, stdout); exit(EXIT_SUCCESS);
    case '?': ERROR_USAGE_EXIT(USAGE_STR_ADD, UNKOWN_OPT_MSG, optopt);
    }

  if (argc < optind + 2)
    ERROR_USAGE_EXIT(
        USAGE_STR_ADD,
        "Error processing args: Expect at least a file and a tag\n");
  opts->file = argv[optind];
  opts->tags_count = (size_t)(argc - optind - 1);
  opts->tags = (const char **)(argv + optind + 1);
}

// -------------------------REMOVE -------------------------
static struct option rm_long_opts[] = {
  {"all", no_argument, NULL, 'a'},
  {"force", no_argument, NULL, 'f'},
  {"strict", no_argument, NULL, 's'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0},
};
void parse_rm_opts(rm_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "afsh", rm_long_opts, NULL)) != -1)
    switch (opt) {
    case 'a': opts->all = true; break;
    case 'f': opts->force = true; break;
    case 's': opts->strict = true; break;
    case 'h': fputs(USAGE_STR_RM, stdout); exit(EXIT_SUCCESS);
    case '?': ERROR_USAGE_EXIT(USAGE_STR_RM, UNKOWN_OPT_MSG, optopt);
    }

  if (opts->all) {
    if (argc < optind + 1)
      ERROR_USAGE_EXIT(USAGE_STR_RM,
                       "Error processing args: Expect at least a file\n");
  } else if (argc < optind + 2)
    ERROR_USAGE_EXIT(
        USAGE_STR_RM,
        "Error processing args: Expect at least a file and a tag\n");

  opts->file = argv[optind];
  opts->tags_count = (size_t)(argc - optind - 1);
  opts->tags = (const char **)(argv + optind + 1);
}

// --------------------------COPY --------------------------
static struct option copy_long_opts[] = {
  {"strict", no_argument, NULL, 's'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0},
};
void parse_copy_opts(copy_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "sh", copy_long_opts, NULL)) != -1)
    switch (opt) {
    case 's': opts->strict = true; break;
    case 'h': fputs(USAGE_STR_COPY, stdout); exit(EXIT_SUCCESS);
    case '?': ERROR_USAGE_EXIT(USAGE_STR_COPY, UNKOWN_OPT_MSG, optopt);
    }

  if (argc < optind + 2)
    ERROR_USAGE_EXIT(
        USAGE_STR_COPY,
        "Error processing args: Expect at least a source and a destination\n");
  if (argc > optind + 2)
    ERROR_USAGE_EXIT(
        USAGE_STR_COPY,
        "Error processing args: Expect at most a source and a destination\n");

  opts->src = argv[optind];
  opts->dst = argv[optind + 1];
}

// -------------------------RENAME -------------------------
static struct option rename_long_opts[] = {
  {"force", no_argument, NULL, 'f'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0},
};
void parse_rename_opts(rename_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "fh", rename_long_opts, NULL)) != -1)
    switch (opt) {
    case 'f': opts->force = true; break;
    case 'h': fputs(USAGE_STR_RENAME, stdout); exit(EXIT_SUCCESS);
    case '?': ERROR_USAGE_EXIT(USAGE_STR_RENAME, UNKOWN_OPT_MSG, optopt);
    }

  if (argc < optind + 2)
    ERROR_USAGE_EXIT(
        USAGE_STR_RENAME,
        "Error processing args: Expect at least an old tag and a new tag\n");
  if (argc > optind + 2)
    ERROR_USAGE_EXIT(
        USAGE_STR_RENAME,
        "Error processing args: Expect at most an old tag and a new tag\n");

  opts->old_tag = argv[optind];
  opts->new_tag = argv[optind + 1];
}

// -------------------------QUERY --------------------------
static struct option query_long_opts[] = {
  {"dir", required_argument, NULL, 'd'},
  {"type", required_argument, NULL, 't'},
  {"mode", required_argument, NULL, 'm'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0},
};
void parse_query_opts(query_opts_t *opts, int argc, char **argv) {
  int opt;
  opts->match_mode = TAG_MATCH_RELEVANCE; // Default match mode
  opts->type = QUERY_TYPE_BOTH;           // Default type

  while ((opt = getopt_long(argc, argv, "d:t:m:h", query_long_opts, NULL)) !=
         -1)
    switch (opt) {
    case 'd': opts->dir = optarg; break;
    case 't':
      if (strcmp(optarg, "file") == 0)
        opts->type = QUERY_TYPE_FILE;
      else if (strcmp(optarg, "dir") == 0)
        opts->type = QUERY_TYPE_DIR;
      else if (strcmp(optarg, "both") == 0)
        opts->type = QUERY_TYPE_BOTH;
      else
        ERROR_USAGE_EXIT(USAGE_STR_QUERY,
                         "Error processing args: Invalid type '%s'\n", optarg);
      break;
    case 'm':
      if (strcmp(optarg, "relevance") == 0)
        opts->match_mode = TAG_MATCH_RELEVANCE;
      else if (strcmp(optarg, "regex") == 0)
        opts->match_mode = TAG_MATCH_REGEX;
      else
        ERROR_USAGE_EXIT(USAGE_STR_QUERY,
                         "Error processing args: Invalid match mode '%s'\n",
                         optarg);
      break;
    case 'h': fputs(USAGE_STR_QUERY, stdout); exit(EXIT_SUCCESS);
    case '?': ERROR_USAGE_EXIT(USAGE_STR_QUERY, UNKOWN_OPT_MSG, optopt);
    }

  switch (opts->match_mode) {
  case TAG_MATCH_RELEVANCE:
    if (argc < optind + 1)
      ERROR_USAGE_EXIT(USAGE_STR_QUERY,
                       "Error processing args: Expect at least a tag\n");

    opts->relevance.tags_count = (size_t)(argc - optind);
    opts->relevance.tags = (const char **)(argv + optind);
    break;
  case TAG_MATCH_REGEX:
    if (argc < optind + 1)
      ERROR_USAGE_EXIT(USAGE_STR_QUERY,
                       "Error processing args: Expect a regex pattern\n");
    if (argc > optind + 1)
      ERROR_USAGE_EXIT(USAGE_STR_QUERY,
                       "Error processing args: Expect only a regex pattern\n");

    opts->regex = argv[optind];
    break;
  }
}

// --------------------------SHOW --------------------------
static struct option show_long_opts[] = {
  {"strict", no_argument, NULL, 's'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0},
};
void parse_show_opts(show_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "sh", show_long_opts, NULL)) != -1)
    switch (opt) {
    case 's': opts->strict = true; break;
    case 'h': fputs(USAGE_STR_SHOW, stdout); exit(EXIT_SUCCESS);
    case '?': ERROR_USAGE_EXIT(USAGE_STR_SHOW, UNKOWN_OPT_MSG, optopt);
    }

  if (argc < optind + 1)
    ERROR_USAGE_EXIT(USAGE_STR_SHOW,
                     "Error processing args: Expect a file or directory\n");
  if (argc > optind + 1)
    ERROR_USAGE_EXIT(
        USAGE_STR_SHOW,
        "Error processing args: Expect at most a file or directory\n");

  opts->file = argv[optind];
}

// --------------------------SYNC --------------------------
static struct option sync_long_opts[] = {
  {"dry-run", no_argument, NULL, 'd'},
  {"deep", no_argument, NULL, 'D'},
  {"yes", no_argument, NULL, 'y'},
  {"help", no_argument, NULL, 'h'},
  {NULL, 0, NULL, 0},
};
void parse_sync_opts(sync_opts_t *opts, int argc, char **argv) {
  int opt;
  while ((opt = getopt_long(argc, argv, "h", sync_long_opts, NULL)) != -1)
    switch (opt) {
    case 'd': opts->dry_run = true; break;
    case 'D': opts->deep = true; break;
    case 'y': opts->yes = true; break;
    case 'h': fputs(USAGE_STR_SYNC, stdout); exit(EXIT_SUCCESS);
    case '?': ERROR_USAGE_EXIT(USAGE_STR_SYNC, UNKOWN_OPT_MSG, optopt);
    }

  if (argc > optind)
    ERROR_USAGE_EXIT(USAGE_STR_SYNC,
                     "Error processing args: Expect no positional arguments\n");
}
