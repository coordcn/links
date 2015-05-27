/**************************************************************
  @author: QianYe(coordcn@163.com)
  @overview: alloc, calloc, realloc, free, memalign from nginx
  @reference: ngx_alloc.c
 **************************************************************/

#include "alloc.h"

void* links_malloc(size_t size){
  void* p;

  p = malloc(size);
  if(p == NULL){
    fprintf(stderr, "malloc(size: %" PRId64 ") failed.\n", size);
  }

  return p;
}

void* links_calloc(size_t size){
  void* p;

  p = links_malloc(size);
  if(p){
    links_memzero(p, size);
  }

  return p;
}

void* links_realloc(void* p, size_t size){
  void* new;

  new = realloc(p, size);
  if(new == NULL){
    fprintf(stderr, "realloc(pointer: %p, size: %" PRId64 ") failed.\n", p, size);
  }

  return new;
}

#if (HAVE_POSIX_MEMALIGN)

void* links_memalign(size_t align, size_t size){
  void* p;

  int err = posix_memalign(&p, align, size);
  if(err){
    fprintf(stderr, "memalign(align: %" PRId64 ", size: %" PRId64 ") failed.\n", align, size);
    p = NULL;
  }

  return p;
}

#elif (HAVE_MEMALIGN)

void* links_memalign(size_t align, size_t size){
  void* p;

  p = memalign(align, size);
  if(p == NULL){
    fprintf(stderr, "memalign(align: %" PRId64 ", size: %" PRId64 ") failed.\n", align, size);
  }

  return p;
}

#endif
