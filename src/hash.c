#include "hash.h"

#include <fts.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "blake3.h"
#include "stb/stb_ds.h"

// 64kb hash chunk
#define HASH_BUF_SIZE (64 * 1024)

int hash_file(const char *path, uint8_t out[32]) {
  FILE *f = fopen(path, "rb");
  if (!f) return -1;

  blake3_hasher hasher;
  blake3_hasher_init(&hasher);

  uint8_t buf[HASH_BUF_SIZE];
  size_t n;

  while ((n = fread(buf, 1, HASH_BUF_SIZE, f)) > 0)
    blake3_hasher_update(&hasher, buf, n);

  if (ferror(f)) {
    fclose(f);
    return -2;
  }

  blake3_hasher_finalize(&hasher, out, 32);
  fclose(f);
  return 0;
}

int fts_compare(const FTSENT **a, const FTSENT **b) {
  return strcmp((*a)->fts_name, (*b)->fts_name);
}

typedef struct {
  blake3_hasher hasher;
} hash_ctx;

int hash_dir(const char *root, uint8_t out[32]) {
  char *paths[] = {(char *)root, NULL};

  FTS *fts = fts_open(paths, FTS_PHYSICAL, fts_compare);
  if (!fts) return -1;

  hash_ctx *stack = NULL;

  FTSENT *ent;
  while ((ent = fts_read(fts)) != NULL)
    switch (ent->fts_info) {
    case FTS_D: {
      // Enter directory → push new hasher
      hash_ctx ctx;
      blake3_hasher_init(&ctx.hasher);
      arrput(stack, ctx);

      // Include directory name in parent (if not root)
      if (arrlen(stack) > 1) {
        hash_ctx *parent = &stack[arrlen(stack) - 2];
        blake3_hasher_update(&parent->hasher, "D", 1);
        blake3_hasher_update(&parent->hasher, ent->fts_name,
                             strlen(ent->fts_name));
        blake3_hasher_update(&parent->hasher, "\0", 1);
      }
      break;
    }
    case FTS_F: {
      hash_ctx *ctx = &arrlast(stack);

      // Include file name
      blake3_hasher_update(&ctx->hasher, "F", 1);
      blake3_hasher_update(&ctx->hasher, ent->fts_name, strlen(ent->fts_name));
      blake3_hasher_update(&ctx->hasher, "\0", 1);

      // Hash file content
      uint8_t file_hash[32];
      if (hash_file(ent->fts_path, file_hash) == 0)
        blake3_hasher_update(&ctx->hasher, file_hash, 32);

      blake3_hasher_update(&ctx->hasher, "\0", 1);
      break;
    }
    case FTS_DP: {
      // Leaving directory → finalize
      hash_ctx ctx = arrpop(stack);

      uint8_t dir_hash[32];
      blake3_hasher_finalize(&ctx.hasher, dir_hash, 32);

      if (arrlen(stack) == 0)
        // root directory
        memcpy(out, dir_hash, 32);
      else {
        // feed into parent
        hash_ctx *parent = &arrlast(stack);
        blake3_hasher_update(&parent->hasher, dir_hash, 32);
        blake3_hasher_update(&parent->hasher, "\0", 1);
      }
      break;
    }
    default:
      // Ignore symlinks, errors, etc. for now
      break;
    }

  fts_close(fts);
  arrfree(stack);
  return 0;
}

void print_hash(const uint8_t hash[32]) {
  for (size_t i = 0; i < 32; i++)
    printf("%02x", hash[i]);
  printf("\n");
}
