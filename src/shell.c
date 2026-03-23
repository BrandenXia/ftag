#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "shell.h"
#include "utils.h"

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
    default:
      ERROR_USAGE_EXIT("Error: Unknown option '-%c'\n", optopt);
    }
}
