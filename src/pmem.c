/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "pmem.h"

#define LINKS_PMEM_ALIGNMENT 64

void links_pmem_init(links_pmem_t* pool, uint32_t max_free_mems){
  links_list_init(&pool->free_list);
  pool->max_free_mems = max_free_mems;
  pool->free_mems = 0;
}

void* links_pmem__alloc(links_pmem_t* pool, size_t size){
  void* buf;
  links_list_t* list;
  links_list_t* free_list = &pool->free_list;

  if(!links_list_is_empty(free_list)){
    list = free_list->next;
    buf = (links_pmem_chunk_t*)links_list_entry(list, links_pmem_chunk_t, list);
    links_list_remove(list);
    pool->free_mems--;  
  }else{
    buf = links_memalign(LINKS_PMEM_ALIGNMENT, size + sizeof(links_pmem_chunk_t));
  }

  return (void*)((char*)buf + sizeof(links_pmem_chunk_t));
}

void links_pmem__free(links_pmem_t* pool, void* p){
  if(p){
    links_pmem_chunk_t* chunk = (links_pmem_chunk_t*)((char*)p - sizeof(links_pmem_chunk_t));

    if(pool->free_mems < pool->max_free_mems){
      links_list_insert_head(&chunk->list, &pool->free_list);
      pool->free_mems++;
    }else{
      links_free(chunk);
    }
  }
}
