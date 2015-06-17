/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#ifndef LINKS_PMEM_H
#define LINKS_PMEM_H

#include "list.h"
#include "alloc.h"
#include "config.h"

#if LINKS_USE_PMEM
#define links_pmem_alloc(pool, size) links_pmem__alloc(pool, size)
#define links_pmem_free(pool, p) links_pmem__free(pool, p)
#else
#define links_pmem_alloc(pool, size) links_malloc(size)
#define links_pmem_free(pool, p) links_free(p)
#endif

typedef struct {
  links_list_t free_list;
  uint32_t max_free_mems;
  uint32_t free_mems;
} links_pmem_t;

typedef struct {
  links_list_t list;
} links_pmem_chunk_t;

void links_pmem_init(links_pmem_t* pool, uint32_t max_free_mems);
void* links_pmem__alloc(links_pmem_t* pool, size_t size);
void links_pmem__free(links_pmem_t* pool, void* p);

#endif /*LINKS_PMEM_H*/
