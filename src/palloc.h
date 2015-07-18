/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#ifndef LINKS_PALLOC_H
#define LINKS_PALLOC_H

#include "list.h"
#include "alloc.h"
#include "config.h"

#if LINKS_USE_PALLOC
#define links_palloc(pool, size) links_pool_alloc(pool, size)
#define links_pfree(pool, p) links_pool_free(pool, p)
#else
#define links_palloc(pool, size) links_malloc(size)
#define links_pfree(pool, p) links_free(p)
#endif

typedef struct {
  links_list_t free_list;
  uint32_t max_free_chunks;
  uint32_t free_chunks;
} links_pool_t;

typedef struct {
  links_list_t list;
} links_pool_chunk_t;

void links_pool_init(links_pool_t* pool, uint32_t max_free_chunks);

static inline void links_pool_set_max_free_chunks(links_pool_t* pool, uint32_t max_free_chunks){
  pool->max_free_chunks = max_free_chunks;
}

void* links_pool_alloc(links_pool_t* pool, size_t size);
void links_pool_free(links_pool_t* pool, void* p);

#endif /*LINKS_PALLOC_H*/
