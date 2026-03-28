#ifndef FTAG_SHELL_H
#define FTAG_SHELL_H

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
    "   copy      Copy tags from one file to another\n"
    "   rename    Rename a tag\n"
    "   query     Query files by tags\n"
    "   find      Alias for 'query --dir .'\n"
    "   show      Show all tags of a file\n"
    "   stat      Show statistics about the tag database\n"
    "   sync      Sync the tag database with the filesystem\n"
    "   cleanup   Cleanup orphaned tags and files from the database\n"
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

static const char *USAGE_STR_COPY =
    "Usage: ftag copy [options] <src> <dst>\n"
    "\n"
    "Options:\n"
    "   -s, --strict    Treat file info mismatch as an error instead of "
    "warning\n"
    "   -h, --help      Show this help message and exit\n";

static const char *USAGE_STR_RENAME =
    "Usage: ftag rename [options] <old_tag> <new_tag>\n"
    "\n"
    "Options:\n"
    "   -f, --force     Force rename even if the new tag already exists\n"
    "                   (Warning: Will merge the two tags if existing!)\n"
    "   -h, --help      Show this help message and exit\n";

static const char *USAGE_STR_QUERY =
    "Usage: ftag query [options] (tag1 [tag2 ...] | [-m regex] REGEX)\n"
    "\n"
    "Options:\n"
    "   -d, --dir DIR     Only search for files under the specified directory "
    "(default: none)\n"
    "   -t, --type        Only search for file or directories (file|dir, "
    "default: both)\n"
    "   -m, --mode MODE   Matching mode (relevance|regex), default: "
    "relevance)\n"
    "   -h, --help        Show this help message and exit\n";

static const char *USAGE_STR_SHOW =
    "Usage: ftag show [options] <file>\n"
    "\n"
    "Options:\n"
    "   -s, --strict    Treat file info mismatch as an error instead of "
    "warning\n"
    "   -h, --help      Show this help message and exit\n";

static const char *USAGE_STR_STAT =
    "Usage: ftag stat [options]\n"
    "\n"
    "Options:\n"
    "   --top-tags-limit N   Show top N tags (default: 5)\n"
    "   -h, --help           Show this help message and exit\n";

static const char *USAGE_STR_SYNC =
    "Usage: ftag sync [options]\n"
    "\n"
    "Options:\n"
    "   --dry-run       Don't actually update the database, just show what "
    "would be done\n"
    "   --deep          Force full hash verification\n"
    "   -y, --yes       Automatically confirm all prompts\n"
    "   -h, --help      Show this help message and exit\n";

static const char *USAGE_STR_CLEANUP =
    "Usage: ftag cleanup [options]\n"
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
  bool strict;
  const char *src;
  const char *dst;
} copy_opts_t;

typedef struct {
  bool force;
  const char *old_tag;
  const char *new_tag;
} rename_opts_t;

typedef struct {
  char *dir;
  enum query_file_type {
    QUERY_TYPE_FILE,
    QUERY_TYPE_DIR,
    QUERY_TYPE_BOTH
  } type;
  enum tag_match_mode { TAG_MATCH_RELEVANCE, TAG_MATCH_REGEX } match_mode;
  union {
    struct {
      size_t tags_count;
      const char **tags;
    } relevance;
    const char *regex;
  };
} query_opts_t;

typedef struct {
  bool strict;
  const char *file;
} show_opts_t;

typedef struct {
  size_t top_tags_limit;
} stat_opts_t;

typedef struct {
  bool dry_run;
  bool deep;
  bool yes;
} sync_opts_t;

typedef struct {
  char not_used; // placeholder since empty struct is not allowed
} cleanup_opts_t;

// clang-format off
void parse_global_opts  (global_opts_t  *, int, char *[]);
void parse_init_opts    (init_opts_t    *, int, char *[]);
void parse_add_opts     (add_opts_t     *, int, char *[]);
void parse_rm_opts      (rm_opts_t      *, int, char *[]);
void parse_copy_opts    (copy_opts_t    *, int, char *[]);
void parse_rename_opts  (rename_opts_t  *, int, char *[]);
void parse_query_opts   (query_opts_t   *, int, char *[]);
void parse_show_opts    (show_opts_t    *, int, char *[]);
void parse_stat_opts    (stat_opts_t    *, int, char *[]);
void parse_sync_opts    (sync_opts_t    *, int, char *[]);
void parse_cleanup_opts (cleanup_opts_t *, int, char *[]);
// clang-format on

#endif
