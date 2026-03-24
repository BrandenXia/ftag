#ifndef FTAG_RESOURCE_H
#define FTAG_RESOURCE_H

#include <sqlite3.h>

#define DATA_DIRNAME ".ftag"

// `path` should be a realpath to a directory, if `data_root` and `data_path`
// are not NULL, they will be set to the parent directory of the data directory
// and the data directory path, respectively
//
// `data_root` and `data_path` must be large enough to hold the resulting paths,
// which will be at most `strlen(path)` and `strlen(path) + sizeof(DATA_DIRNAME)
// + 1` (for '/'), respectively
void find_data_dir(const char *path, char *data_root, char *data_path);

// Return the data directory path given the data root
//
// This function returns a dynamically allocated string that must be freed by
// the caller
char *get_data_path(const char *data_root);

// Return the database file path given the data path
//
// This function returns a dynamically allocated string that must be freed by
// the caller
char *get_db_path(const char *data_path);

typedef struct {
  char *data_path;
  char *data_root;
  char *db_path;
  sqlite3 *db;
} resources_t;

#endif
