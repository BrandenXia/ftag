#include "resource.h"

#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "db.h"
#include "str_utils.h"
#include "utils.h"

bool check_data_dir(const char *path) {
  struct stat st;
  int err = stat(path, &st);
  if (err != 0) {
    if (errno == ENOENT)
      return false;
    else
      ERROR_EXIT("Error checking path '%s': %s", path, strerror(errno));
  }

  if (!S_ISDIR(st.st_mode)) {
    printf("Warning: '%s' exists but is not a directory, skipping\n", path);
    return false;
  }

  return true;
}

void find_data_dir(const char *path, char *data_root, char *data_dir) {
  size_t path_len = strlen(path);
  char path_buf[path_len + 1];
  snprintf(path_buf, sizeof(path_buf), "%s", path);
  // buf2 is both for constructing the parent path and checking the data dir
#define MAX(a, b) ((a) > (b) ? (a) : (b))
  char path_buf2[path_len + MAX(sizeof(DATA_DIRNAME), 3) + 1]; // 3 for "/.."
#undef MAX

  struct stat st_cwd, st_parent;
  while (true) {
    if (stat(path_buf, &st_cwd) != 0)
      ERROR_EXIT("Error checking path '%s': %s", path_buf, strerror(errno));

    size_t buf_len = strlcpy(path_buf2, path_buf, sizeof(path_buf2));
    strlcat(path_buf2, "/" DATA_DIRNAME, sizeof(path_buf2));
    if (check_data_dir(path_buf2)) {
      if (data_root != NULL) strlcpy(data_root, path_buf, path_len + 1);
      if (data_dir != NULL)
        strlcpy(data_dir, path_buf2, path_len + sizeof(DATA_DIRNAME) + 1);
      return; // found the data dir
    }

    // now, remove the last component of path_buf2 and replace with "/.."
    char *path_buf2_end = path_buf2 + buf_len + 1;
    // the remaining space of buf2 is what's left after the original path and
    // the '/' we added
    strlcpy(path_buf2_end, "..", sizeof(path_buf2) - buf_len - 1);
    char *res = realpath(path_buf2, path_buf);
    if (!res)
      ERROR_EXIT("Error resolving parent path '%s': %s", path_buf2,
                 strerror(errno));

    if (stat(path_buf, &st_parent) != 0)
      ERROR_EXIT("Error checking path '%s': %s", path_buf, strerror(errno));

    if (st_cwd.st_dev == st_parent.st_dev && st_cwd.st_ino == st_parent.st_ino)
      break;
  }

  ERROR_EXIT("Error finding data directory: reached root directory without "
             "finding the data directory\nPlease make sure you are inside a "
             "valid project directory and the data directory is correctly "
             "initialized.\n");
}

char *get_data_path(const char *data_root) {
  size_t len = strlen(data_root) + sizeof(DATA_DIRNAME) + 1;
  char *path = malloc(len);
  snprintf(path, len, "%s/" DATA_DIRNAME, data_root);
  return path;
}

char *get_db_path(const char *data_path) {
  if (!data_path) ERROR_EXIT("Data path is NULL");

  size_t len = strlen(data_path) + sizeof(DB_FILENAME) + 1;
  char *db_path = malloc(len);
  if (!db_path) PERROR_EXIT("Memory allocation failed for db_path");

  snprintf(db_path, len, "%s/" DB_FILENAME, data_path);

  return db_path;
}
