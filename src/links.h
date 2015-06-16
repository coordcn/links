/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#ifndef LINKS_LINKS_H
#define LINKS_LINKS_H

#include "lua.h"
#include "lauxlib.h"
#include "uv.h"

#include "config.h"

#include "list.h"
#include "alloc.h"
#include "palloc.h"
#include "buf.h"

#define CR '\r'
#define LF '\n'
#define CRLF "\r\n"

/*init.c*/
lua_State* links_get_main_thread();

/*util.c*/
int links_cannot_change(lua_State* L);
void links_uv_error(lua_State* L, int err); 

#endif /*LINKS_LINKS_H*/
