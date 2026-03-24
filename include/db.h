#ifndef FTAG_DB_H
#define FTAG_DB_H

#include <stdbool.h>

#include <sqlite3.h>

#define DB_FILENAME "db.sqlite3"

sqlite3 *init_db(const char *path, bool verbose);

#endif
