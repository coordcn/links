/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "pbuf.h"

#define LINKS_PBUF_MAGIC 0xAA55AA55

static links_pool_t links_pbuf_slots[LINKS_PBUF_SLOT_SIZE];

void links_pbuf_init(uint32_t max_free_chunks){
  for(int i = 0; i < LINKS_PBUF_SLOT_SIZE; i++){
    links_pool_init(&links_pbuf_slots[i], max_free_chunks);
  }
}

void links_pbuf_set_max_free_chunks(size_t slot, uint32_t max_free_chunks){
  if(slot < LINKS_PBUF_SLOT_SIZE){
    links_pbuf_slots[slot].max_free_chunks = max_free_chunks;
  }
}

void* links_pbuf__alloc(size_t size){
  if(size > LINKS_PBUF_MAX_CHUNK_SIZE){
    links_pool_chunk_t* chunk = links_memalign(LINKS_POOL_ALIGNMENT, size + sizeof(links_pool_chunk_t));
    if(!chunk) return NULL;

    chunk->magic = LINKS_PBUF_MAGIC;
    return (void*)((char*)chunk + sizeof(links_pool_chunk_t));
  }

  size_t slot = links_pbuf_slot(size);
  return links_pool_alloc(&links_pbuf_slots[slot], size, slot); 
}

void links_pbuf__free(void* p){
  if(p){
    links_pool_chunk_t* chunk = (links_pool_chunk_t*)((char*)p - sizeof(links_pool_chunk_t));
    if(chunk->magic == LINKS_PBUF_MAGIC){
      links_free(chunk);
      return;
    }
   
    assert(chunk->magic != LINKS_POOL_MAGIC);
    assert(chunk->magic < LINKS_PBUF_SLOT_SIZE);

    links_pool_free(&links_pbuf_slots[chunk->magic], p);
  }
}
