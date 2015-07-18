/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "palloc.h"

#define LINKS_POOL_ALIGNMENT 64

void links_pool_init(links_pool_t* pool, uint32_t max_free_chunks){
  links_list_init(&pool->free_list);
  pool->max_free_chunks = max_free_chunks;
  pool->free_chunks = 0;
}

void* links_pool_alloc(links_pool_t* pool, size_t size){
  void* chunk;
  links_list_t* list;
  links_list_t* free_list = &pool->free_list;

  if(!links_list_is_empty(free_list)){
    list = free_list->next;
    chunk = (links_pool_chunk_t*)links_list_entry(list, links_pool_chunk_t, list);
    links_list_remove(list);
    pool->free_chunks--;  
  }else{
    chunk = links_memalign(LINKS_POOL_ALIGNMENT, size + sizeof(links_pool_chunk_t));
  }

  return (void*)((char*)chunk + sizeof(links_pool_chunk_t));
}

void links_pool_free(links_pool_t* pool, void* p){
  if(p){
    links_pool_chunk_t* chunk = (links_pool_chunk_t*)((char*)p - sizeof(links_pool_chunk_t));

    if(pool->free_chunks < pool->max_free_chunks){
      links_list_insert_head(&chunk->list, &pool->free_list);
      pool->free_chunks++;
    }else{
      links_free(chunk);
    }
  }
}
