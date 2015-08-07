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

/*node.js*/
static void links_platform_init(){
  /*ignore SGPIPE*/
  struct sigaction sa;
  links_memzero(&sa, sizeof(struct sigaction));
  sa.sa_handler = SIG_IGN;
  int ret = sigaction(SIGPIPE, &sa, NULL);
  assert(ret == 0);
}

int links_init(lua_State *L, int argc, char* argv[]){
  links_platform_init();
  links_buf_slot_init();
  links_pbuf_init(LINKS_PBUF_MAX_FREE_CHUNKS);
  links_thread_ref_hash_init(LINKS_8K_SHIFT);
  links_tcp_socket_pool_init(LINKS_TCP_SOCKET_POOL_MAX_FREE_CHUNKS);
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
  
  /*tcp*/
  lua_pushcfunction(L, luaopen_tcp);
  lua_setfield(L, -2, "tcp_native");
  
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
