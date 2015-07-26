/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "links.h"

static links_hash_t* links_thread_ref_hash;

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

void links_thread_ref_hash_init(size_t bits){
  links_thread_ref_hash = links_hash_create_int_value(bits);
}

void links_thread_ref_hash_set(lua_State* L, int ref){
  links_hash_int_set_value(links_thread_ref_hash, (size_t)L, ref);
}

int links_thread_ref_hash_get(lua_State* L, int* ref){
  return links_hash_int_get_value(links_thread_ref_hash, (size_t)L, ref);
}

void links_resume(lua_State* L, int nargs, int thread_ref){
  int ret = lua_resume(L, NULL, nargs);
  const char* error;
  int ref = LUA_NOREF;

  switch(ret){
    case LUA_YIELD:
      return;
    case LUA_OK:
      links_hash_int_get_remove_value(links_thread_ref_hash, (size_t)L, &ref);

      assert(ref == thread_ref);

      if(thread_ref != LUA_NOREF){
        luaL_unref(L, LUA_REGISTRYINDEX, thread_ref);
      }

      return;
    case LUA_ERRRUN:
      error = lua_tostring(L, -1);
      luaL_error(L, "lua_resume() LUA_ERRRUN error: %s\n", error ? error : "unknow"); 
      return;
    case LUA_ERRMEM:
      error = lua_tostring(L, -1);
      luaL_error(L, "lua_resume() LUA_ERRMEM error: %s\n", error ? error : "unknow"); 
      return;
    case LUA_ERRERR:
      error = lua_tostring(L, -1);
      luaL_error(L, "lua_resume() LUA_ERRERR error: %s\n", error ? error : "unknow"); 
      return;
    case LUA_ERRGCMM:
      error = lua_tostring(L, -1);
      luaL_error(L, "lua_resume() LUA_ERRGCMM error: %s\n", error ? error : "unknow"); 
      return;
    default:
      error = lua_tostring(L, -1);
      luaL_error(L, "lua_resume() unknow error: %s\n", error ? error : "unknow"); 
  }
}
