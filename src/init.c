/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include <stdlib.h>
#include "system.h"
#include "process.h"
#include "http.h"
#include "init.h"

static char links_main_thread;
static char links_main_loop;

int links_init(lua_State *L){
  uv_loop_t* loop = uv_default_loop();

  lua_pushthread(L);
  lua_rawsetp(L, LUA_REGISTRYINDEX, &links_main_thread);
  
  lua_pushlightuserdata(L, loop);
  lua_rawsetp(L, LUA_REGISTRYINDEX, &links_main_loop);

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
  
  /*http*/
  lua_pushcfunction(L, luaopen_http);
  lua_setfield(L, -2, "http");

  lua_pop(L, 1);

  return 0;
}

lua_State* links_get_main_thread(lua_State *L){
  lua_State *main_thread;
  
  lua_rawgetp(L, LUA_REGISTRYINDEX, &links_main_thread);
  main_thread = lua_tothread(L, -1);
  lua_pop(L, 1);
  
  return main_thread;
}

uv_loop_t* links_get_main_loop(lua_State *L) {
  uv_loop_t *loop;
  
  lua_rawgetp(L, LUA_REGISTRYINDEX, &links_main_loop);
  loop = lua_touserdata(L, -1);
  lua_pop(L, 1);
  
  return loop;
}
