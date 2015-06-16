/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: memory pool for small memory less than 1024 bytes.
             the memory is used for a short time. 
             if the memory is used for a long time,
             please use links_alloc(alloc.h native malloc).
 **************************************************************/

#ifndef LINKS_PALLOC_H
#define LINKS_PALLOC_H

#include "list.h"
#include "alloc.h"
#include "config.h"

#if LINKS_USE_PALLOC
#define links_palloc(size) links_pool_alloc(size)
#define links_pfree(p) links_pool_free(p)
#else
#define links_palloc(size) links_malloc(size)
#define links_pfree(p) links_free(p)
#endif

typedef struct {
  links_list_t node;
  links_list_t free_list;
  size_t slot;
  size_t max_chunks;
  size_t used_chunks;
} links_pool_block_t;

typedef struct {
  links_pool_block_t* block;
  size_t magic;
} links_pool_chunk_t;

typedef struct {
  links_list_t free_list;
  links_list_t full_list;
  links_pool_block_t* current;
  size_t max_free_blocks;
  size_t free_blocks;
  size_t max_chunks_shift;
} links_pool_slot_t;

void links_pool_slot_init();
void links_pool_set_max_free_blocks(size_t slot, size_t size);
void links_pool_set_max_chunks_shift(size_t slot, size_t shift);

/*size unit is bytes*/
void* links_pool_alloc(size_t size);
void links_pool_free(void* p);

#endif /*LINKS_PALLOC_H*/
