/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#ifndef LINKS_BUF_H
#define LINKS_BUF_H

#include "list.h"
#include "config.h"

#define LINKS_BUF_SLOT_SIZE 16
#define LINKS_BUF_STEP_SIZE 16
#define LINKS_BUF_STEP_SIZE_SHIFT LINKS_16_SHIFT
#define LINKS_BUF_MAX_SIZE (LINKS_BUF_STEP_SIZE * LINKS_BUF_SLOT_SIZE)
#define links_buf_slot(size) ((links_align(LINKS_BUF_STEP_SIZE, (size)) >> LINKS_BUF_STEP_SIZE_SHIFT) - 1)

typedef struct {
  links_list_t list;
  size_t slot;
  size_t size;
  char* start;
  char* end;
  char* pos;
  char* last;
} links_buf_t;

typedef struct {
  links_list_t free_list;
  size_t buf_size;
  uint32_t max_free_bufs;
  uint32_t free_bufs;
} links_buf_slot_t;

void links_buf_slot_init();
void links_buf_set_max_free_bufs(size_t size, uint32_t max_free_bufs);

/*size unit is kbytes*/
links_buf_t* links_buf_alloc(size_t size);
void links_buf_free(links_buf_t* buf);

#endif /*LINKS_BUF_H*/
