#ifndef FTAG_SHELL_H
#define FTAG_SHELL_H

#include <getopt.h>
#include <stdbool.h>
#include <stddef.h>

#include "db.h"

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
    "   query     Query files by tags\n"
    "   find      Alias for 'query --dir .'\n"
    "   show      Show all tags of a file\n"
    "   sync      Sync the tag database with the filesystem\n"
    "\n"
    "To get help for a specific command, run 'ftag <command> --help'\n";

static const char *USAGE_STR_INIT =
    "Usage: ftag init [options] [directory]\n"
    "\n"
    "Options:\n"
    "   -f, --force     Force reinitialize even if a database already exists\n"
    "                   (Warning: This will erase all existing tags!)\n"
    "   -h, --help      Show this help message and exit\n"
    "\n"
    "Arguments:\n"
    "   directory     The directory to initialize the tag database in\n"
    "                 (default: current directory)\n";

static const char *USAGE_STR_ADD =
    "Usage: ftag add [options] <file> tag1 [tag2 ...]\n"
    "\n"
    "Options:\n"
    "   -f, --force     Don't report error if the tag already exists\n"
    "   -s, --strict    Treat file info mismatch as an error instead of "
    "warning\n"
    "   -h, --help      Show this help message and exit\n";

static const char *USAGE_STR_RM =
    "Usage: ftag rm [options] <file> tag1 [tag2 ...]\n"
    "\n"
    "Options:\n"
    "   -a, --all       Remove all tags from the file\n"
    "   -f, --force     Don't report error if the tag does not exist\n"
    "   -s, --strict    Treat file info mismatch as an error instead of "
    "warning\n"
    "   -h, --help      Show this help message and exit\n";

static const char *USAGE_STR_QUERY =
    "Usage: ftag query [options] tag1 [tag2 ...]\n"
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
    "   -s, --strict    Treat file info mismatch as an error instead of "
    "warning\n"
    "   -h, --help      Show this help message and exit\n";

static const char *USAGE_STR_SYNC =
    "Usage: ftag sync [options]\n"
    "\n"
    "Options:\n"
    "   --dry-run       Don't actually update the database, just show what "
    "would be done\n"
    "   --deep          Force full hash verification\n"
    "   -y, --yes       Automatically confirm all prompts\n"
    "   -h, --help      Show this help message and exit\n";

typedef struct {
  bool verbose;
} global_opts_t;

typedef struct {
  bool force;
  const char *dir;
} init_opts_t;

typedef struct {
  bool strict;
  const char *file;
  size_t tags_count;
  const char **tags;
} add_opts_t;

typedef struct {
  bool all;
  bool force;
  bool strict;
  const char *file;
  size_t tags_count;
  const char **tags;
} rm_opts_t;

typedef struct {
  char *dir;
  enum query_type { QUERY_TYPE_FILE, QUERY_TYPE_DIR, QUERY_TYPE_BOTH } type;
  enum tag_match_mode match_mode;
  size_t tags_count;
  const char **tags;
} query_opts_t;

typedef struct {
  bool strict;
  const char *file;
} show_opts_t;

typedef struct {
  bool dry_run;
  bool deep;
  bool yes;
} sync_opts_t;

// clang-format off
void parse_global_opts (global_opts_t *, int, char *[]);
void parse_init_opts   (init_opts_t   *, int, char *[]);
void parse_add_opts    (add_opts_t    *, int, char *[]);
void parse_rm_opts     (rm_opts_t     *, int, char *[]);
void parse_query_opts  (query_opts_t  *, int, char *[]);
void parse_show_opts   (show_opts_t   *, int, char *[]);
void parse_sync_opts   (sync_opts_t   *, int, char *[]);
// clang-format on

#endif
