#include "utils.h"

#include <ftw.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// from https://stackoverflow.com/a/5467788
// TODO: replace with fts api as suggested by the Man Page of nftw
static int unlink_cb(const char *fpath, const struct stat *, int,
                     struct FTW *) {
  int rv = remove(fpath);
  if (rv) perror(fpath);
  return rv;
}
int rmrf(const char *path) {
  return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

char *get_relative_path(const char *from, const char *to) {
  if (!from || !to) return NULL;

  if ((from[0] == '/') != (to[0] == '/')) return NULL;

  const char *f = from;
  const char *t = to;

  while (*f && *t) {
    const char *next_f = strchr(f, '/');
    const char *next_t = strchr(t, '/');

    if (!next_f) next_f = f + strlen(f);
    if (!next_t) next_t = t + strlen(t);

    size_t len_f = (size_t)(next_f - f);
    size_t len_t = (size_t)(next_t - t);

    if (len_f == len_t && strncmp(f, t, len_f) == 0) {
      f = next_f;
      t = next_t;
      if (*f == '/') f++;
      if (*t == '/') t++;
    } else
      break;
  }

  int up_count = 0;
  if (*f != '\0') {
    up_count = 1;
    for (const char *p = f; *p; p++)
      if (*p == '/') up_count++;
  }

  if (up_count == 0 && *t == '\0') return strdup(".");

  size_t result_len = (size_t)(up_count * 3) + strlen(t) + 1;
  char *result = malloc(result_len);
  if (!result) return NULL;

  char *ptr = result;
  for (int i = 0; i < up_count; i++) {
    memcpy(ptr, "../", 3);
    ptr += 3;
  }

  if (*t == '\0' && up_count > 0)
    *(ptr - 1) = '\0';
  else
    strcpy(ptr, t);

  return result;
}

void get_file_info(const char *path, file_info_t *info) {
  struct stat st;
  if (stat(path, &st) != 0) PERROR_EXIT("Error getting file info");

  info->is_dir = S_ISDIR(st.st_mode);
  info->size = st.st_size;
  info->mtime = st.st_mtime;
}

char *concat_paths(const char *base, const char *relative, char *buf,
                   size_t buf_size) {
  if (!base || !relative) return NULL;

  size_t base_len = strlen(base);
  int needs_sep = base_len > 0 && base[base_len - 1] != '/';
  // +1 for optional '/', +1 for '\0'
  size_t needed = base_len + (size_t)needs_sep + strlen(relative) + 1;

  char *out;
  if (buf) {
    if (buf_size < needed) return NULL;
    out = buf;
  } else {
    out = malloc(needed);
    if (!out) return NULL;
  }

  // snprintf handles the '\0' terminator
  snprintf(out, needed, needs_sep ? "%s/%s" : "%s%s", base, relative);

  return out;
}
