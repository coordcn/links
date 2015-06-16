/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "alloc.h"

char* links__strndup(const char* src, size_t n){
  char* dst;

  dst = links_malloc(n + 1);
  if(!dst){
    return NULL;
  }

  dst[n] = '\0';
  links_memcpy(dst, src, n);

  return dst;
}
