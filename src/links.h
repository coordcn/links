/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#ifndef LINKS_LINKS_H
#define LINKS_LINKS_H

#include "lua.h"
#include "lauxlib.h"
#include "uv.h"

#include "config.h"

#include "list.h"
#include "alloc.h"
#include "palloc.h"
#include "pbuf.h"
#include "buf.h"
#include "hash.h"
#include "links_errno.h"

#define CR '\r'
#define LF '\n'
#define CRLF "\r\n"

/*int64_t*/
#define links_integer lua_Integer
/*double*/
#define links_number lua_Number

/*init.c*/
lua_State* links_get_main_thread();

/*links.c*/
int links_cannot_change(lua_State* L);

void links_uv_error(lua_State* L, int err);
void links_error(lua_State* L, int code, const char* name, const char* msg);

void links_thread_ref_hash_init(size_t bits);
void links_thread_ref_hash_set(lua_State* L, int ref);
int links_thread_ref_hash_get(lua_State* L, int* ref);

void links_resume(lua_State* L, int nargs, int thread_ref);

#endif /*LINKS_LINKS_H*/
