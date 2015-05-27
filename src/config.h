/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#ifndef LINKS_CONFIG_H
#define LINKS_CONFIG_H

#include "linux_config.h"

#define links_align(size, alignment) (((size) + (alignment - 1)) & ~(alignment - 1))

#define LINKS_BITS 64
#define LINKS_POINTER_SIZE (sizeof(void*))

#define LINKS_2_SHIFT 1
#define LINKS_4_SHIFT 2
#define LINKS_8_SHIFT 3
#define LINKS_16_SHIFT 4
#define LINKS_32_SHIFT 5
#define LINKS_64_SHIFT 6
#define LINKS_128_SHIFT 7
#define LINKS_256_SHIFT 8
#define LINKS_512_SHIFT 9
#define LINKS_1K_SHIFT 10
#define LINKS_2K_SHIFT 11
#define LINKS_4K_SHIFT 12
#define LINKS_8K_SHIFT 13
#define LINKS_16K_SHIFT 14
#define LINKS_32K_SHIFT 15
#define LINKS_64K_SHIFT 16

#define LINKS_USE_PALLOC 1

#endif /*LINKS_CONFIG_H*/
