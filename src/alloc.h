/**************************************************************
  @author: QianYe(coordcn@163.com)
  @overview: alloc, calloc, realloc, free, memalign from nginx
  @reference: ngx_alloc.h
 **************************************************************/

#ifndef LINKS_ALLOC_H
#define LINKS_ALLOC_H

#include "config.h"

#define links_lower(c)      (u_char) ((c >= 'A' && c <= 'Z') ? (c | 0x20) : c)
#define links_upper(c)      (u_char) ((c >= 'a' && c <= 'z') ? (c & ~0x20) : c)

#define links_memzero(p, size) memset(p, 0, size)
#define links_memset(p, c, size) memset(p, c, size)

#define links_memcpy(dst, src, n) memcpy(dst, src, n)
#define links_cpymem(dst, src, n) (((u_char *) memcpy(dst, src, n)) + (n))

#define links_memcmp(s1, s2, n) memcmp((const char*)s1, (const char*)s2, n)

#define links_strncmp(s1, s2, n) strncmp((const char*)s1, (const char*)s2, n)
#define links_strncasecmp(s1, s2, n) strncasecmp((const char*)s1, (const char*)s2, n)
#define links_strndup(src, n) links__strndup((const char*)src, n)

char* links__strndup(const char* src, size_t n);

void* links_malloc(size_t size);
void* links_calloc(size_t size);
void* links_realloc(void* p, size_t size);
#define links_free  free

/**
  Linux has memalign() or posix_memalign()
  Solaris has memalign()
  FreeBSD 7.0 has posix_memalign(), besides, early version's malloc()
  aligns allocations bigger than page size at the page boundary
 */

#define HAVE_POSIX_MEMALIGN 1
/*#define HAVE_MEMALIGN 1*/

#if (HAVE_POSIX_MEMALIGN || HAVE_MEMALIGN)

void* links_memalign(size_t align, size_t size);

#else

#define links_memalign(align, size) links_alloc(size)

#endif


#endif /*LINKS_ALLOC_H*/
