#ifndef FTAG_DB_H
#define FTAG_DB_H

#include <stdbool.h>

#define DB_DIRNAME ".ftag"
#define DB_FILENAME "db.sqlite3"

void init_db(const char *path, bool verbose);

#endif
