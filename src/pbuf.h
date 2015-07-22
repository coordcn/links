/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#ifndef LINKS_PBUF_H
#define LINKS_PBUF_H

#include "alloc.h"
#include "config.h"
#include "palloc.h"

#define LINKS_PBUF_SLOT_SIZE 16
#define LINKS_PBUF_CHUNK_STEP 1024
#define LINKS_PBUF_CHUNK_STEP_SHIFT LINKS_1K_SHIFT
#define LINKS_PBUF_MAX_CHUNK_SIZE (LINKS_PBUF_CHUNK_STEP * LINKS_PBUF_SLOT_SIZE)
#define links_pbuf_slot(size) ((links_align(LINKS_PBUF_CHUNK_STEP, size) >> LINKS_PBUF_CHUNK_STEP_SHIFT) - 1)

#if LINKS_USE_PBUF
#define links_pbuf_alloc(size) links_pbuf__alloc(size)
#define links_pbuf_free(p) links_pbuf__free(p)
#else
#define links_pbuf_alloc(size) links_malloc(size)
#define links_pbuf_free(p) links_free(p)
#endif

void links_pbuf_init(uint32_t max_free_chunks);
void links_pbuf_set_max_free_chunks(size_t slot, uint32_t max_free_chunks);

void* links_pbuf__alloc(size_t size);
void links_pbuf__free(void* p);

#endif /*LINKS_PBUF_H*/
