#include "hash.h"

#include <fts.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "blake3.h"

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

typedef struct {
  hash_ctx *items;
  size_t size;
  size_t capacity;
} sstack_t;

void stack_init(sstack_t *s) {
  s->items = NULL;
  s->size = 0;
  s->capacity = 0;
}

void stack_push(sstack_t *s, hash_ctx ctx) {
  if (s->size == s->capacity) {
    s->capacity = s->capacity ? s->capacity * 2 : 8;
    s->items = realloc(s->items, s->capacity * sizeof(hash_ctx));
  }
  s->items[s->size++] = ctx;
}

hash_ctx *stack_top(sstack_t *s) { return &s->items[s->size - 1]; }
hash_ctx stack_pop(sstack_t *s) { return s->items[--s->size]; }

void stack_free(sstack_t *s) { free(s->items); }

int hash_dir(const char *root, uint8_t out[32]) {
  char *paths[] = {(char *)root, NULL};

  FTS *fts = fts_open(paths, FTS_PHYSICAL, fts_compare);
  if (!fts) return -1;

  sstack_t stack;
  stack_init(&stack);

  FTSENT *ent;
  while ((ent = fts_read(fts)) != NULL)
    switch (ent->fts_info) {
    case FTS_D: {
      // Enter directory → push new hasher
      hash_ctx ctx;
      blake3_hasher_init(&ctx.hasher);
      stack_push(&stack, ctx);

      // Include directory name in parent (if not root)
      if (stack.size > 1) {
        hash_ctx *parent = &stack.items[stack.size - 2];
        blake3_hasher_update(&parent->hasher, "D", 1);
        blake3_hasher_update(&parent->hasher, ent->fts_name,
                             strlen(ent->fts_name));
        blake3_hasher_update(&parent->hasher, "\0", 1);
      }
      break;
    }
    case FTS_F: {
      hash_ctx *ctx = stack_top(&stack);

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
      hash_ctx ctx = stack_pop(&stack);

      uint8_t dir_hash[32];
      blake3_hasher_finalize(&ctx.hasher, dir_hash, 32);

      if (stack.size == 0)
        // root directory
        memcpy(out, dir_hash, 32);
      else {
        // feed into parent
        hash_ctx *parent = stack_top(&stack);
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
  stack_free(&stack);
  return 0;
}

void print_hash(const uint8_t hash[32]) {
  for (size_t i = 0; i < 32; i++)
    printf("%02x", hash[i]);
  printf("\n");
}
