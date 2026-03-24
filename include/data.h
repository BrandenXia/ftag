#ifndef FTAG_DATA_H
#define FTAG_DATA_H

#define DATA_DIRNAME ".ftag"

// path should be a realpath to a directory
//
// data_dir must be large enough to hold the resulting path, which will be at
// most strlen(path) + sizeof(DATA_DIRNAME) + 1 (for '/')
void find_data_dir(const char *path, char *data_dir);

#endif
