/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h> /* PATH_MAX */

#include "init.h"

int main(int argc, char *argv[]) {
  lua_State *L;

  argv = uv_setup_args(argc, argv);
  
  L = luaL_newstate();
  if (L == NULL) {
    fprintf(stderr, "Cannot create lua_State: Not enough memory.\n");
    return 1;
  }
  
  luaL_openlibs(L);

  if (links_init(L)) {
    fprintf(stderr, "links_init(L) failed.\n");
    return 1;
  }
  
  const char* fname = argv[1];
  if (luaL_dofile(L, fname)) {
    printf("%s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
    lua_close(L);
    return -1;
  }
  
  lua_close(L);
  return 0;
}
