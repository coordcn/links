/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include <stdlib.h>
#include "http.h"

static int links_http_create_server(lua_State* L){
  printf("test hello.");

  return 1;
}

int luaopen_http(lua_State* L){
  luaL_Reg lib[] = {
    { "createServer", links_http_create_server},
    { NULL, NULL}
  };

  luaL_newlib(L, lib);

  return 1;
}
