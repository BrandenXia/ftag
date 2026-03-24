#ifndef FTAG_RESOURCE_H
#define FTAG_RESOURCE_H

#include <sqlite3.h>

#define DATA_DIRNAME ".ftag"

// path should be a realpath to a directory
//
// data_dir must be large enough to hold the resulting path, which will be at
// most strlen(path) + sizeof(DATA_DIRNAME) + 1 (for '/')
void find_data_dir(const char *path, char *data_dir);

// Return the data path found from the current working directory
//
// This function returns a dynamically allocated string that must be freed by
// the caller
char *get_data_path(void);

// Return the parent directory of the data directory given the data path
//
// This function returns a dynamically allocated string that must be freed by
// the caller
char *get_data_parent(const char *data_path);

// Return the database file path given the data path
//
// This function returns a dynamically allocated string that must be freed by
// the caller
char *get_db_path(const char *data_path);

typedef struct {
  char *data_path;
  char *data_parent;
  char *db_path;
  sqlite3 *db;
} resources_t;

void setup_resources(resources_t *r, bool verbose);
void cleanup_resources(resources_t *r);

#endif
