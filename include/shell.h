#ifndef FTAG_SHELL_H
#define FTAG_SHELL_H

#include "db.h"
#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>

static const char *USAGE_STR_GLOBAL =
    "Usage: ftag [options] <command> [args...]\n"
    "\n"
    "Options:\n"
    "   -v, --verbose   Enable verbose output (for debugging)\n"
    "   -h, --help      Show this help message and exit\n"
    "\n"
    "Commands:\n"
    "   init      Initalize a new tag database under the current directory\n"
    "   add       Add tags to a file\n"
    "   rm        Remove tags from a file\n"
    "   find      Find files by tags\n"
    "   show      Show all tags of a file\n"
    "   sync      Sync the tag database with the filesystem"
    "\n"
    "To get help for a specific command, run 'ftag <command> --help'\n";

static const char *USAGE_STR_INIT =
    "Usage: ftag init [options] [directory]\n"
    "\n"
    "Options:\n"
    "   -h, --help      Show this help message and exit\n"
    "   -f, --force     Force reinitialize even if a database already exists\n"
    "                   (Warning: This will erase all existing tags!)\n"
    "\n"
    "Arguments:\n"
    "   directory     The directory to initialize the tag database in\n"
    "                 (default: current directory)\n";

static const char *USAGE_STR_ADD =
    "Usage: ftag add [options] <file> tag1 [tag2 ...]\n"
    "\n"
    "Options:\n"
    "   -h, --help      Show this help message and exit\n"
    "   -f, --force     Don't report error if the tag already exists\n";

static const char *USAGE_STR_RM =
    "Usage: ftag rm [options] <file> tag1 [tag2 ...]\n"
    "\n"
    "Options:\n"
    "   -h, --help      Show this help message and exit\n"
    "   -a, --all       Remove all tags from the file\n"
    "   -f, --force     Don't report error if the tag does not exist\n";

static const char *USAGE_STR_FIND =
    "Usage: ftag find [options] tag1 [tag2 ...]\n"
    "\n"
    "Options:\n"
    "   -d, --dir DIR     Only search for files under the specified directory "
    "(default: none)\n"
    "   -t, --type        Only search for file or directories (file|dir, "
    "default: both)\n"
    "   -m, --match MODE  Matching mode (and|or|relevance, default: "
    "relevance)\n"
    "   -h, --help        Show this help message and exit\n";

static const char *USAGE_STR_SHOW =
    "Usage: ftag show [options] <file>\n"
    "\n"
    "Options:\n"
    "   -h, --help      Show this help message and exit\n";

static const char *USAGE_STR_SYNC =
    "Usage: ftag sync [options]\n"
    "\n"
    "Options:\n"
    "   -h, --help      Show this help message and exit\n";

typedef struct {
  bool verbose;
} global_opts_t;

typedef struct {
  bool force;
  const char *dir;
} init_opts_t;

typedef struct {
  const char *file;
  size_t tags_count;
  const char **tags;
} add_opts_t;

typedef struct {
  bool all;
  bool force;
  const char *file;
  size_t tags_count;
  const char **tags;
} rm_opts_t;

typedef struct {
  char *dir;
  enum find_type { FIND_TYPE_FILE, FIND_TYPE_DIR, FIND_TYPE_BOTH } type;
  enum tag_match_mode match_mode;
  size_t tags_count;
  const char **tags;
} find_opts_t;

typedef struct {
} show_opts_t;

typedef struct {
} sync_opts_t;

// clang-format off
void parse_global_opts (global_opts_t *, int, char *[]);
void parse_init_opts   (init_opts_t   *, int, char *[]);
void parse_add_opts    (add_opts_t    *, int, char *[]);
void parse_rm_opts     (rm_opts_t     *, int, char *[]);
void parse_find_opts   (find_opts_t   *, int, char *[]);
void parse_show_opts   (show_opts_t   *, int, char *[]);
void parse_sync_opts   (sync_opts_t   *, int, char *[]);
// clang-format on

#endif
