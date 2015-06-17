/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "alloc.h"
#include "buf.h"

#define LINKS_BUF_SLOT_SIZE 16
#define LINKS_BUF_STEP_SIZE 16
#define LINKS_BUF_STEP_SIZE_SHIFT LINKS_16_SHIFT
#define LINKS_BUF_MAX_SIZE (LINKS_BUF_STEP_SIZE * LINKS_BUF_SLOT_SIZE)
#define links_buf_slot(size) ((links_align(LINKS_BUF_STEP_SIZE, (size)) >> LINKS_BUF_STEP_SIZE_SHIFT) - 1)

static size_t links_pagesize;
static links_buf_slot_t links_buf_slots[LINKS_BUF_SLOT_SIZE]; 

void links_buf_slot_init(){
  links_pagesize = getpagesize();
  
  for(int i = 0; i < LINKS_BUF_SLOT_SIZE; i++){
    links_list_init(&links_buf_slots[i].free_list);
    links_buf_slots[i].buf_size = (LINKS_BUF_STEP_SIZE * (i + 1)) << LINKS_1K_SHIFT;
    links_buf_slots[i].max_free_bufs = 1 << LINKS_1K_SHIFT;
  }
}

void links_buf_set_max_free_bufs(size_t size, uint32_t max_free_bufs){
  if(size > LINKS_BUF_MAX_SIZE){
    fprintf(stderr, "links_buf_set_max_free_bufs(size: %" PRId64 ", max_free_bufs: %d) size must be less than %d.\n", size, max_free_bufs, LINKS_BUF_MAX_SIZE);
    return;
  }
  
  size_t slot = links_buf_slot(size);
  links_buf_slots[slot].max_free_bufs = max_free_bufs;
}

links_buf_t* links_buf_alloc(size_t size){
  if(size > LINKS_BUF_MAX_SIZE){
    fprintf(stderr, "links_buf_alloc(size: %" PRId64 ") size must be less than %d.\n", size, LINKS_BUF_MAX_SIZE);
    return NULL;
  }
  
  size_t slot = links_buf_slot(size);

  links_buf_t* buf;
  links_list_t* list;
  
  links_list_t* free_list = &links_buf_slots[slot].free_list;
  if(!links_list_is_empty(free_list)){
    list = free_list->next;
    buf = (links_buf_t*)links_list_entry(list, links_buf_t, list);
    links_list_remove(list);
    links_buf_slots[slot].free_bufs--;  
  }else{
    size_t buf_size = links_buf_slots[slot].buf_size;
    buf = links_memalign(links_pagesize, buf_size);
    buf->slot = slot;
    buf->start = ((char*)buf + sizeof(links_buf_t));
    buf->pos = buf->start;
    buf->last = buf->start;
    buf->end = (char*)buf + buf_size;
    buf->size = buf->end - buf->start;
  }

  /*links_list_init(&buf->list);*/
  return buf;
}

void links_buf_free(links_buf_t* buf){
  if(buf){
    size_t slot = buf->slot;
    if(links_buf_slots[slot].free_bufs < links_buf_slots[slot].max_free_bufs){
      links_list_insert_head(&buf->list, &links_buf_slots[slot].free_list);
      buf->pos = buf->start;
      buf->last = buf->start;
      links_buf_slots[slot].free_bufs++;
    }else{
      links_free(buf);
    }
  }
}
