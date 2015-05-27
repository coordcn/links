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

typedef struct {
  links_list_t list;
  size_t slot;
  char* start;
  char* end;
  char* pos;
  char* last;
} links_buf_t;

typedef struct {
  links_list_t free_list;
  size_t buf_size;
  size_t max_free_bufs;
  size_t free_bufs;
} links_buf_slot_t;

void links_buf_slot_init();
void links_buf_set_max_free_bufs(size_t slot, size_t size);

/*size unit is kbytes*/
links_buf_t* links_buf_alloc(size_t size);
void links_buf_free(links_buf_t* buf);

#endif /*LINKS_BUF_H*/
