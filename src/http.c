/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "init.h"
#include "links.h"

static int links_http_create_server(lua_State* L){
  printf("test hello.");

  return 1;
}

/*static int links_cannot_change(lua_State* L){*/
/*return luaL_error(L, "table fields cannot be changed.");*/
/*}*/

int luaopen_http(lua_State *L){
  luaL_Reg lib[] = {
    { "createServer", links_http_create_server},
    { "__newindex", links_cannot_change},
    { NULL, NULL}
  };

  lua_createtable(L, 0, 0);

  luaL_newlib(L, lib);
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_pushliteral(L, "metatable is protected.");
  lua_setfield(L, -2, "__metatable");

  lua_setmetatable(L, -2);

  return 1;
}
