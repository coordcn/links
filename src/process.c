/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "process.h"

int luaopen_process(lua_State *L){
  luaL_Reg lib[] = {
    { NULL, NULL}
  };

  luaL_newlib(L, lib);

  return 1;
}
