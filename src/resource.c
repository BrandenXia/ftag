#include "resource.h"

#include <dirent.h>
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "db.h"
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

void find_data_dir(const char *path, char *data_dir) {
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

char *get_data_path(void) {
  char *cwd = realpath(".", NULL);
  if (!cwd)
    PERROR_EXIT("Error resolving current working directory");

  size_t len = strlen(cwd) + sizeof(DATA_DIRNAME) + 1;
  char *data_path = malloc(len);
  if (!data_path)
    PERROR_EXIT("Memory allocation failed for data_path");

  find_data_dir(cwd, data_path);
  free(cwd);

  return data_path;
}

char *get_data_parent(const char *data_path) {
  char *path = malloc(strlen(data_path) - sizeof("/" DATA_DIRNAME) + 2);
  strncpy(path, data_path, strlen(data_path) - sizeof("/" DATA_DIRNAME) + 1);
  return path;
}

char *get_db_path(const char *data_path) {
  if (!data_path)
    ERROR_EXIT("Data path is NULL");

  size_t len = strlen(data_path) + sizeof(DB_FILENAME) + 1;
  char *db_path = malloc(len);
  if (!db_path)
    PERROR_EXIT("Memory allocation failed for db_path");

  snprintf(db_path, len, "%s/" DB_FILENAME, data_path);

  return db_path;
}

void setup_resources(resources_t *r, bool verbose) {
  r->data_path = get_data_path();
  r->data_parent = get_data_parent(r->data_path);
  if (verbose)
    printf("Data path: %s\n", r->data_path);
  char *db_path = get_db_path(r->data_path);
  r->db = init_db(db_path, verbose);
}

void cleanup_resources(resources_t *r) {
  sqlite3_close(r->db);
  free(r->db_path);
  free(r->data_parent);
  free(r->data_path);
}
