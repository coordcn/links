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
