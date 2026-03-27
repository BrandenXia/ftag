#include <string.h>

#include <sqlite3.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include "pcre2.h"

#define REGEX_MATCH_LIMIT 100000 // regex backtrack step limit

static void free_pcre2_regex(void *re) { pcre2_code_free((pcre2_code *)re); }

static void sqlite_regex_func(sqlite3_context *ctx, int argc,
                              sqlite3_value **argv) {
  (void)argc; // unused parameter

  const unsigned char *pattern = sqlite3_value_text(argv[0]);
  const unsigned char *text = sqlite3_value_text(argv[1]);

  if (!pattern || !text) {
    sqlite3_result_null(ctx);
    return;
  }

  pcre2_code *re = (pcre2_code *)sqlite3_get_auxdata(ctx, 0);

  if (re == NULL) {
    int errornumber;
    PCRE2_SIZE erroroffset;
    re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED,
                       PCRE2_ANCHORED | PCRE2_ENDANCHORED | PCRE2_UTF |
                           PCRE2_UCP,
                       &errornumber, &erroroffset, NULL);

    if (re == NULL) {
      sqlite3_result_error(ctx, "Regex compilation failed", -1);
      return;
    }

    pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);
    sqlite3_set_auxdata(ctx, 0, re, free_pcre2_regex);
  }

  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

  pcre2_match_context *mcontext = pcre2_match_context_create(NULL);
  pcre2_set_match_limit(mcontext, REGEX_MATCH_LIMIT);
  int rc =
      pcre2_match(re, text, PCRE2_ZERO_TERMINATED, 0, 0, match_data, mcontext);

  pcre2_match_context_free(mcontext);
  pcre2_match_data_free(match_data);

  if (rc >= 0)
    sqlite3_result_int(ctx, 1);
  else if (rc == PCRE2_ERROR_NOMATCH)
    sqlite3_result_int(ctx, 0);
  else if (rc == PCRE2_ERROR_MATCHLIMIT)
    sqlite3_result_error(ctx, "Regex is too complex (hit backtrack limit)", -1);
  else
    sqlite3_result_error(ctx, "Regex execution error", -1);
}

int register_regexp_extension(sqlite3 *db) {
  return sqlite3_create_function(db, "regexp", 2,
                                 SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL,
                                 sqlite_regex_func, NULL, NULL);
}
