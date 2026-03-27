#ifndef FTAG_UTILS_H
#define FTAG_UTILS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ERROR_EXIT(...)                                                        \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define PERROR_EXIT(msg)                                                       \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

#define ERROR_USAGE_EXIT(USAGE, ...)                                           \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    fputs(USAGE, stderr);                                                      \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

int rmrf(const char *path);

/**
 * Computes the relative path from one path to another. Note: Both inputs MUST
 * be relative to the same root (e.g. both absolute or both relative to the same
 * directory). returned string is dynamically allocated and must be free()'d by
 * the caller.
 */
char *get_relative_path(const char *from, const char *to);

typedef struct {
  bool is_dir;
  int64_t size;
  int64_t mtime;
} file_info_t;

void get_file_info(const char *path, file_info_t *info);

// Concatenate two paths, an optional buf can be provided to avoid allocation if
// the caller can guarantee it's large enough.
// Returns a pointer to the resulting string, which is either the provided buf
// or the newly allocated string (if buf is NULL). The caller is responsible for
// free()'ing the returned string if buf is NULL.
char *concat_paths(const char *base, const char *relative, char *buf,
                   size_t buf_size);

#endif
