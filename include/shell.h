#ifndef FTAG_SHELL_H
#define FTAG_SHELL_H

#include <getopt.h>
#include <stdbool.h>

static const char *USAGE_STR_GLOBAL =
    "Usage: ftag [options] <command> [args...]\n"
    "\n"
    "Options:\n"
    "   -v, --verbose   Enable verbose output\n"
    "   -h, --help      Show this help message and exit\n"
    "\n"
    "Commands:\n"
    "   init      Initalize a new tag database under the current directory\n"
    "   add       Add tags to a file\n"
    "   rm        Remove tags from a file\n"
    "   find      Find files by tags\n"
    "\n"
    "To get help for a specific command, run 'ftag <command> --help'\n";

typedef struct {
  bool verbose;
} global_opts_t;

void parse_global_opts(global_opts_t *opts, int argc, char *argv[]);

#endif
