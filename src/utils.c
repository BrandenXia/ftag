#include "utils.h"

#include <ftw.h>
#include <stdio.h>
#include <unistd.h>

// from https://stackoverflow.com/a/5467788
// TODO: replace with fts api as suggested by the Man Page of nftw
static int unlink_cb(const char *fpath, const struct stat *, int,
                     struct FTW *) {
  int rv = remove(fpath);
  if (rv)
    perror(fpath);
  return rv;
}
int rmrf(const char *path) {
  return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}
