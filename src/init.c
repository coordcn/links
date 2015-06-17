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

  /*raise the open file descriptor limit*/
  struct rlimit limit;
  if(getrlimit(RLIMIT_NOFILE, &limit) == 0 && limit.rlim_cur != limit.rlim_max){
    /*binary search*/
    rlim_t min = limit.rlim_cur;
    rlim_t max = 1 << 20;
    if(limit.rlim_max != RLIM_INFINITY){
      min = limit.rlim_max;
      max = limit.rlim_max;
    }
    do {
      limit.rlim_cur = min + (max - min) / 2;
      if(setrlimit(RLIMIT_NOFILE, &limit)){
        max = limit.rlim_cur;
      }else{
        min = limit.rlim_cur;
      }
    }while(min + 1 < max);
  }
}

int links_init(lua_State *L, int argc, char* argv[]){
  links_platform_init();
  links_pool_slot_init();
  links_buf_slot_init();
  links_tcp_socket_pool_init(1024);
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
  lua_setfield(L, -2, "tcp");
  
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
