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

#define links_memset        memset
#define links_memcpy        memcpy
#define links_memmove       memmove
#define links_memcmp        memcmp
#define links_memzero(p, size) links_memset(p, 0, size)

#define links_strncmp       strncmp
#define links_strncasecmp   strncasecmp
#define links_strndup       links__strndup

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

#define links_memalign(align, size) links_malloc(size)

#endif


#endif /*LINKS_ALLOC_H*/
