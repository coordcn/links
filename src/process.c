/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "process.h"

static int links_process_fork(lua_State* L){
  printf("test hello.");

  return 1;
}

int luaopen_process(lua_State *L){
  luaL_Reg lib[] = {
    { "fork", links_process_fork},
    { NULL, NULL}
  };

  luaL_newlib(L, lib);

  return 1;
}
