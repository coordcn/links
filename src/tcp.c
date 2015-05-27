/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "init.h"
#include "links.h"

void links_tcp_on_connection(uv_stream_t* handle, int status){
}

void links_tcp_on_alloc(uv_handle_t* hanle, size_t suggested_size, uv_buf_t* buf){
}

void links_tcp_on_read(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf){
}

void links_tcp_after_shutdown(uv_shutdown_t* req, int status){
}

void links_tcp_after_write(uv_write_t* req, int status){
}

void links_tcp_try_write(uv_buf_t** bufs, size_t* count){
}

void links_tcp_write(uv_buf_t* bufs, size_t count){
}

int luaopen_tcp(lua_State *L){
  luaL_Reg lib[] = {
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
