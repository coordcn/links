/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "init.h"
#include "links.h"

static uint64_t links_start_time;
static lua_State* links_main_thread;

int links_init(lua_State *L, int argc, char* argv[]){
  links_pool_slot_init();
  links_buf_slot_init();
  links_dns_init();

  links_start_time = uv_now(uv_default_loop());
  links_main_thread = L;

  /*preload*/
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "preload");
  lua_remove(L, -2);

  /*system*/
  lua_pushcfunction(L, luaopen_system);
  lua_setfield(L, -2, "system");
 
  /*process*/
  lua_pushcfunction(L, luaopen_process);
  lua_setfield(L, -2, "process");
  
  /*dns*/
  lua_pushcfunction(L, luaopen_dns);
  lua_setfield(L, -2, "dns");
  
  /*http*/
  lua_pushcfunction(L, luaopen_http);
  lua_setfield(L, -2, "http");

  lua_pop(L, 1);

  /*argv*/
  lua_createtable (L, argc, 0);
  for(int i = 0; i < argc; i++) {
    lua_pushstring (L, argv[i]);
    lua_rawseti(L, -2, i + 1);
  }
  lua_setglobal(L, "argv");

  return 0;
}

lua_State* links_get_main_thread(){
  return links_main_thread;
}

uint64_t links_get_start_time(){
  return links_start_time;
}
