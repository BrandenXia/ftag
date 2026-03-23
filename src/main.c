#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"
#include "utils.h"

typedef int (*cmd_func_t)(int argc, char *argv[], global_opts_t *);

int cmd_init(int, char *[], global_opts_t *) { return 0; }
int cmd_add(int, char *[], global_opts_t *) { return 0; }
int cmd_rm(int, char *[], global_opts_t *) { return 0; }
int cmd_find(int, char *[], global_opts_t *) { return 0; }

int main(int argc, char *argv[]) {
  global_opts_t opts = {0};

  parse_global_opts(&opts, argc, argv);

  if (optind >= argc)
    ERROR_USAGE_EXIT("Error: Expect a subcommand\n", stderr);

  char *subcmd = argv[optind];
  cmd_func_t cmd_func;
#define CMP(str) (strcmp(subcmd, str) == 0)
  if CMP ("init")
    cmd_func = cmd_init;
  else if CMP ("add")
    cmd_func = cmd_add;
  else if CMP ("rm")
    cmd_func = cmd_rm;
  else if CMP ("find")
    cmd_func = cmd_find;
#undef CMP
  else
    ERROR_USAGE_EXIT("Error: Unknown subcommand '%s'\n", subcmd);

  optind = 1; // Reset optind for subcommand parsing
  return cmd_func(argc - optind, argv + optind, &opts);
}
