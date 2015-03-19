/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#ifndef LINKS_INIT_H
#define LINKS_INIT_H

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "uv.h"

int links_init(lua_State *L);

lua_State* links_get_main_thread(lua_State *L);
uv_loop_t* links_get_main_loop(lua_State *L);

#endif /*LINKS_INIT_H*/
