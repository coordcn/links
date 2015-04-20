/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#ifndef LINKS_INIT_H
#define LINKS_INIT_H

#include "lua.h"
#include "lauxlib.h"
#include "uv.h"

int links_init(lua_State *L);

uint64_t links_get_start_time();

int luaopen_system(lua_State *L);
int luaopen_process(lua_State *L);
int luaopen_http(lua_State* L);

#endif /*LINKS_INIT_H*/
