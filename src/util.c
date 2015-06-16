/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "links.h"

int links_cannot_change(lua_State* L){
  return luaL_error(L, "table fields cannot be changed.");
}

void links_uv_error(lua_State* L, int err){
  lua_createtable(L, 0, 3);
  lua_pushinteger(L, err);
  lua_setfield(L, -2, "code");
  lua_pushstring(L, uv_err_name(err));
  lua_setfield(L, -2, "name");
  lua_pushstring(L, uv_strerror(err));
  lua_setfield(L, -2, "message");
}
