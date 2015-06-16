/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "palloc.h"

#define LINKS_POOL_ALIGNMENT 64
#define LINKS_POOL_BLOCKS_SIZE (links_align(LINKS_POOL_ALIGNMENT, sizeof(links_pool_block_t)))

#define LINKS_POOL_SLOT_SIZE 16
#define LINKS_POOL_CHUNK_STEP 64
#define LINKS_POOL_CHUNK_STEP_SHIFT LINKS_64_SHIFT
#define LINKS_POOL_MAX_CHUNK_SIZE (LINKS_POOL_CHUNK_STEP * LINKS_POOL_SLOT_SIZE - sizeof(links_pool_chunk_t))
#define links_pool_slot(size) ((links_align(LINKS_POOL_CHUNK_STEP, (size + sizeof(links_pool_chunk_t))) >> LINKS_POOL_CHUNK_STEP_SHIFT) - 1)

#define LINKS_POOL_MAX_FREE_BLOCKS 64
#define LINKS_POOL_MAX_CHUNKS_SHIFT 6

#define LINKS_POOL_MAGIC 0xAA55AA55

static links_pool_slot_t links_pool_slots[LINKS_POOL_SLOT_SIZE];

void links_pool_slot_init(){
  for(int i = 0; i < LINKS_POOL_SLOT_SIZE; i++){
    links_list_init(&links_pool_slots[i].free_list);
    links_list_init(&links_pool_slots[i].full_list);
    links_pool_slots[i].max_free_blocks = LINKS_POOL_MAX_FREE_BLOCKS;
    links_pool_slots[i].max_chunks_shift = LINKS_POOL_MAX_CHUNKS_SHIFT;
  }
}

void links_pool_set_max_free_blocks(size_t slot, size_t size){
  if(slot < LINKS_POOL_SLOT_SIZE){
    links_pool_slots[slot].max_free_blocks = size;
  }
}

void links_pool_set_max_chunks_shift(size_t slot, size_t shift){
  if(slot < LINKS_POOL_SLOT_SIZE){
    links_pool_slots[slot].max_chunks_shift = shift;
  }
}

static links_pool_block_t* links_pool_create_block(size_t slot, size_t chunks_shift){
  size_t slot_1 = slot + 1;
  size_t shift = LINKS_POOL_CHUNK_STEP_SHIFT + chunks_shift;
  size_t block_size = LINKS_POOL_BLOCKS_SIZE + (slot_1 << shift);
  size_t chunk_size = slot_1 << LINKS_POOL_CHUNK_STEP_SHIFT;
  size_t chunk_num = 1 << chunks_shift;

  links_pool_block_t* block;
  links_list_t* free_list;
  char* start;
  
  block = links_memalign(LINKS_POOL_ALIGNMENT, block_size);
  if(!block){
    return NULL;
  }
        
  links_list_init(&block->node);
  links_list_init(&block->free_list);
  block->slot = slot;
  block->max_chunks = chunk_num;
  block->used_chunks = 0;
        
  free_list = &block->free_list;
  start = (char*)block + LINKS_POOL_BLOCKS_SIZE;
  for(size_t i = 0; i < chunk_num; i++){
    links_list_insert_tail((links_list_t*)start, free_list);
    start += chunk_size;
  }

  return block;
}

void* links_pool_alloc(size_t size){
  links_pool_chunk_t* chunk;
  if(size > LINKS_POOL_MAX_CHUNK_SIZE){
    chunk = links_memalign(LINKS_POOL_ALIGNMENT, size);
    chunk->block = NULL;
    chunk->magic = LINKS_POOL_MAGIC;
    return (void*)((char*)chunk + sizeof(links_pool_chunk_t));
  }

  size_t slot = links_pool_slot(size);

  links_pool_block_t* current = links_pool_slots[slot].current;
  links_list_t* free_list = &links_pool_slots[slot].free_list;
  links_list_t* full_list = &links_pool_slots[slot].full_list;
  size_t max_chunks_shift = links_pool_slots[slot].max_chunks_shift;
  if(current){
    if(links_list_is_empty(&current->free_list)){
      /*assert(current.used_chunks == current.max_chunks);*/
      links_list_insert_head(&current->node, full_list);
      if(!links_list_is_empty(free_list)){
        current = links_list_entry(free_list->next, links_pool_block_t, node);
        links_pool_slots[slot].current = current;
        links_pool_slots[slot].free_blocks--;
      }else{
        current = links_pool_create_block(slot, max_chunks_shift);
        if(!current){
          fprintf(stderr, "links_pool_alloc(size: %" PRId64 ") out of memory.\n", size);
          return NULL;
        }

        links_pool_slots[slot].current = current;
      }
    }
  }else{
    current = links_pool_create_block(slot, max_chunks_shift);
    if(!current){
      fprintf(stderr, "links_pool_alloc(size: %" PRId64 ") out of memory.\n", size);
      return NULL;
    }

    links_pool_slots[slot].current = current;
  }

  links_list_t* node = current->free_list.next;
  links_list_remove(node);
  current->used_chunks++;
  chunk = (links_pool_chunk_t*)node;
  chunk->block = current;
  chunk->magic = 0;

  return (void*)((char*)node + sizeof(links_pool_chunk_t));
}

void links_pool_free(void* p){
  if(p){
    links_pool_chunk_t* chunk = (links_pool_chunk_t*)((char*)p - sizeof(links_pool_chunk_t));
    if(chunk->magic == LINKS_POOL_MAGIC){
      links_free(chunk);
      return;
    }
    
    assert(chunk->magic == 0);

    links_pool_block_t* block = chunk->block;
    links_list_insert_head((links_list_t*)chunk, &block->free_list);
    block->used_chunks--;

    if(block->used_chunks == 0){
      size_t slot = block->slot;
      if(links_pool_slots[slot].current != block){
        if(links_pool_slots[slot].free_blocks < links_pool_slots[slot].max_free_blocks){
          links_list_insert_head(&block->node, &links_pool_slots[slot].free_list);
          links_pool_slots[slot].free_blocks++;
        }else{
          links_free(block);
        }
      }
    }
  }
}
