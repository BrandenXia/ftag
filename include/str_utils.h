#ifndef FTAG_STR_UTILS_H
#define FTAG_STR_UTILS_H

#include <stddef.h>

#if defined(__GNUC__) || defined(__clang__)
#define WEAK_ATTR __attribute__((weak))
#else
#define WEAK_ATTR
#endif

#ifndef strlcpy
/*
 * Copy string src to buffer dst of size dsize.  At most dsize-1
 * chars will be copied.  Always NUL terminates (unless dsize == 0).
 * Returns strlen(src); if retval >= dsize, truncation occurred.
 */
WEAK_ATTR size_t strlcpy(char *dst, const char *src, size_t dsize);
#endif

#ifndef strlcat
/*
 * Appends src to string dst of size dsize (unlike strncat, dsize is the
 * full size of dst, not space left).  At most dsize-1 characters
 * will be copied.  Always NUL terminates (unless dsize <= strlen(dst)).
 * Returns strlen(src) + MIN(dsize, strlen(initial dst)).
 * If retval >= dsize, truncation occurred.
 */
WEAK_ATTR size_t strlcat(char *dst, const char *src, size_t dsize);
#endif

#endif
