#ifndef FTAG_HASH_H
#define FTAG_HASH_H

#include <stdint.h>

int hash_file(const char *path, uint8_t out[32]);

int hash_dir(const char *path, uint8_t out[32]);

void print_hash(const uint8_t hash[32]);

#endif
