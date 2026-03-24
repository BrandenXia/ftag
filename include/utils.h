#ifndef FTAG_UTILS_H
#define FTAG_UTILS_H

#include <stdio.h>
#include <stdlib.h>

#include "shell.h"

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

#define ERROR_USAGE_EXIT(...)                                                  \
  do {                                                                         \
    fprintf(stderr, __VA_ARGS__);                                              \
    fputs(USAGE_STR_GLOBAL, stderr);                                           \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

int rmrf(const char *path);

#endif
