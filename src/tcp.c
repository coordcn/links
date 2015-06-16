/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "init.h"
#include "links.h"

/*read error*/
#define LINKS_READABLE_SHIFT 1
/*write error*/
#define LINKS_WRITEABLE_SHIFT 2
/*socket:shutdown()*/
#define LINKS_SHUTDOWN_SHIFT 3
/*socket:close()*/
#define LINKS_CLOSE_SHIFT 4

#define links_set_flag(flag, shift) (flag |= (1 << shift))
#define links_clear_flag(flag, shift) (flag &= ~(1 << shift))
#define links_check_flag(flag, shift) (flag & (1 << shift))

typedef struct {
  uv_tcp_t handle;
  lua_State* thread;
  size_t maxconnections;
  size_t connections;
  size_t timeout;
  int thread_ref;
  int self_ref;
  int onconnect_ref;
  size_t max_read_bytes;
  size_t max_write_bytes;
  uint32_t read_buf_size;
  uint32_t write_buf_size;
} links_tcp_server_t;

typedef union {
  uv_connect_t connect_req;
  uv_shutdown_t shutdown_req;
  uv_write_t write_req;
} links_tcp_req_t;

typedef struct {
  size_t test;
  uv_tcp_t handle;
  uv_timer_t timer;
  links_tcp_req_t req;
  size_t timeout;
  links_tcp_server_t* server;
  lua_State* thread;
  links_buf_t* read_buf;
  int thread_ref;
  int self_ref;
  int write_data_ref;
  uint32_t read_buf_size;
  size_t read_bytes; 
  size_t write_bytes;
  size_t bytes; /* read or write bytes */
  size_t flag;
} links_tcp_socket_t;

static char links_tcp_server_metatable_key;
static char links_tcp_socket_metatable_key;

/*static void links_tcp_socket_onclose(uv_handle_t* handle);*/

static void links_resume(lua_State* co, int nargs, int self_ref, int thread_ref, const char* name){
  int ret = lua_resume(co, NULL, nargs);
  const char* error;

  switch(ret){
    case LUA_YIELD:
      return;
    case LUA_OK:
      luaL_unref(co, LUA_REGISTRYINDEX, self_ref);
      luaL_unref(co, LUA_REGISTRYINDEX, thread_ref);
      return;
    case LUA_ERRRUN:
      error = lua_tostring(co, -1);
      luaL_error(co, "lua_resume() LUA_ERRRUN error: %s\n", error ? error : "unknow"); 
      return;
    case LUA_ERRMEM:
      error = lua_tostring(co, -1);
      luaL_error(co, "lua_resume() LUA_ERRMEM error: %s\n", error ? error : "unknow"); 
      return;
    case LUA_ERRERR:
      error = lua_tostring(co, -1);
      luaL_error(co, "lua_resume() LUA_ERRERR error: %s\n", error ? error : "unknow"); 
      return;
    case LUA_ERRGCMM:
      error = lua_tostring(co, -1);
      luaL_error(co, "lua_resume() LUA_ERRGCMM error: %s\n", error ? error : "unknow"); 
      return;
    default:
      error = lua_tostring(co, -1);
      luaL_error(co, "lua_resume() unknow error: %s\n", error ? error : "unknow"); 
  }
}

static void links_tcp_server_onconnect(uv_stream_t* handle, int status){
  if(status){
    fprintf(stderr, "server onconnect status error: %s\n", uv_strerror(status));
    return;
  }

  links_tcp_server_t* server = container_of(handle, links_tcp_server_t, handle);
  if(server->connections >= server->maxconnections){
    fprintf(stderr, "server reached max connections: %" PRId64 "\n", server->maxconnections);
    return;
  }

  lua_State* L = server->thread;
  lua_State* co = lua_newthread(L);
  int thread_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  /*onconnect function*/
  lua_rawgeti(co, LUA_REGISTRYINDEX, server->onconnect_ref);

  /*socket*/
  links_tcp_socket_t** client_ptr = lua_newuserdata(co, sizeof(links_tcp_socket_t*));

  *client_ptr = links_malloc(sizeof(links_tcp_socket_t));
  links_tcp_socket_t* client = *client_ptr;

  /*socket metatable*/
  lua_pushlightuserdata(co, &links_tcp_socket_metatable_key);
  lua_rawget(co, LUA_REGISTRYINDEX);
  lua_setmetatable(co, -2);

  uv_tcp_t* client_handle = &client->handle;
  uv_loop_t* loop = uv_default_loop();
  uv_tcp_init(loop, client_handle);
  int err = uv_accept(handle, (uv_stream_t*)client_handle);
  if(err){
    luaL_unref(L, LUA_REGISTRYINDEX, thread_ref);
    links_free(client);
    fprintf(stderr, "uv_accept() error: %s\n", uv_strerror(err));
    return;
  }
  
  server->connections++;
  /*fprintf(stderr, "connections: %d\n", server->connections);*/
    /*uv_close((uv_handle_t*)client_handle, links_tcp_socket_onclose);*/

  /*uv_timer_init(loop, &client->timer);*/
  client->timeout = server->timeout;
  client->server = server;
  client->thread = co;
  client->thread_ref = LUA_NOREF;
  lua_pushvalue(co, -1);
  /*client->self_ref = LUA_NOREF;*/
  client->self_ref = luaL_ref(co, LUA_REGISTRYINDEX);
  client->write_data_ref = LUA_NOREF;
  client->read_buf_size = 16;
  client->read_buf = NULL;
  client->read_bytes = 0;
  client->write_bytes = 0;
  client->bytes = 0;
  client->flag = 0;
  links_set_flag(client->flag, LINKS_READABLE_SHIFT);
  links_set_flag(client->flag, LINKS_WRITEABLE_SHIFT);

  lua_rawgeti(co, LUA_REGISTRYINDEX, server->self_ref);
  /*lua_xmove(L, co, 3);*/
  links_resume(co, 2, client->self_ref, thread_ref, "connect");
}

/**
  tcp = require('tcp')
  onconnection = require('xxx')
  local options = {
    port =,
    host =,
    family = 4,
    maxconnections =,
    timeout =,
    tcp_nodelay = true,
    tcp_keepalive =, 
    tcp_backlog = 1024,
    tcp_recvbuf =,
    tcp_sendbuf =,
    tcp_reuseport = true,
    onconnect = function(socket){
    },
  }
  tcp.createServer(options)
 */
static int links_tcp_create_server(lua_State* L){
  if(lua_type(L, 1) != LUA_TTABLE){
    return luaL_argerror(L, 1, "tcp.createServer(options) error: options must be [table]\n"); 
  }

  lua_getfield(L, 1, "port");
  int port;
  if(lua_type(L, -1) == LUA_TNUMBER){
    port = lua_tointeger(L, -1);
  }else{
    return luaL_argerror(L, 2, "tcp.createServer(options) error: options.port must be [number]\n"); 
  }

  if(port < 0 || port > 65535){
    return luaL_argerror(L, 2, "tcp.createServer(options) error: options.port must be [0, 65535]\n"); 
  }
  lua_pop(L, 1);

  lua_getfield(L, 1, "family");
  int family;
  if(lua_type(L, -1) == LUA_TNUMBER){
    family = lua_tointeger(L, -1);
  }else if(lua_type(L, -1) == LUA_TNIL){
    family = 4;
  }else{
    return luaL_argerror(L, 2, "tcp.createServer(options) error: options.family must be [number]\n"); 
  }

  if((family != 4) && (family != 6)){
    return luaL_argerror(L, 2, "tcp.createServer(options) error: options.family must be 4 or 6\n"); 
  }
  lua_pop(L, 1);

  lua_getfield(L, 1, "host");
  const char* host;
  if(lua_type(L, -1) == LUA_TSTRING){
    host = lua_tostring(L, -1);
  }else if(lua_type(L, -1) == LUA_TNIL){
    host = (family == 4) ? "0.0.0.0" : "::";
  }else{
    return luaL_argerror(L, 2, "tcp.createServer(options) error: options.host must be [string]\n"); 
  }

  struct sockaddr_in addr4;
  struct sockaddr_in6 addr6;
  struct sockaddr* addr;
  if(uv_ip4_addr(host, port, &addr4) == 0){
    addr = (struct sockaddr*)(&addr4);
  }else if(uv_ip6_addr(host, port, &addr6) == 0){
    addr = (struct sockaddr*)(&addr6);
  }else{
    return luaL_argerror(L, 2, "tcp.createServer(options) error: options.host is not a IP address\n"); 
  }
  lua_pop(L, 1);

  lua_getfield(L, 1, "onconnect");
  int onconnect_ref;
  if(lua_type(L, -1) == LUA_TFUNCTION){
    onconnect_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  }else{
    return luaL_argerror(L, 2, "tcp.createServer(options) error: options.onconnect must be [function]\n"); 
  }

  lua_getfield(L, 1, "tcp_backlog");
  int tcp_backlog;
  if(lua_type(L, -1) == LUA_TNUMBER){
    tcp_backlog = lua_tointeger(L, -1);
  }else if(lua_type(L, -1) == LUA_TNIL){
    /*backlog = roundup_pow_of_two(tcp_backlog + 1)*/
    tcp_backlog = 511;
  }else{
    return luaL_argerror(L, 2, "tcp.createServer(options) error: options.tcp_backlog must be [number]\n"); 
  }
  lua_pop(L, 1);
  
  links_tcp_server_t* server = lua_newuserdata(L, sizeof(links_tcp_server_t));
  lua_pushlightuserdata(L, &links_tcp_server_metatable_key);
  lua_rawget(L, LUA_REGISTRYINDEX);
  lua_setmetatable(L, -2);

  uv_tcp_t* handle = &server->handle;
  uv_tcp_init(uv_default_loop(), handle);
  int err = uv_tcp_bind(handle, addr, 0);
  if(err){
    return luaL_error(L, "tcp.createServer(options) uv_tcp_bind() error: %s\n", uv_strerror(err)); 
  }

  err = uv_listen((uv_stream_t*)handle, tcp_backlog, links_tcp_server_onconnect);
  if(err){
    return luaL_error(L, "tcp.createServer(options) uv_listen() error: %s\n", uv_strerror(err)); 
  }

  server->maxconnections = 65536;
  server->connections = 0;
  server->timeout = 0;
  server->thread = L;
  server->thread_ref = LUA_NOREF;
  if(L != links_get_main_thread()){
    lua_pushthread(L);  
    server->thread_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  }
  server->self_ref = luaL_ref(L, LUA_REGISTRYINDEX);
  server->onconnect_ref = onconnect_ref;

  return 0;
}

static void links_tcp_server_onclose(uv_handle_t* handle){
  links_tcp_server_t* server = container_of(handle, links_tcp_server_t, handle);
  lua_State* L = links_get_main_thread();

  int thread_ref = server->thread_ref;
  luaL_unref(L, LUA_REGISTRYINDEX, server->onconnect_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, server->self_ref);
  luaL_unref(L, LUA_REGISTRYINDEX, thread_ref);
}

static int links_tcp_server_close(lua_State* L){
  if(lua_type(L, 1) != LUA_TUSERDATA){
    return luaL_argerror(L, 1, "server:close() error: server must be [userdata]\n"); 
  }

  links_tcp_server_t* server = lua_touserdata(L, 1);
  if(!server){
    return luaL_argerror(L, 1, "server:close() error: server is closed\n"); 
  }

  uv_handle_t* handle = (uv_handle_t*)(&server->handle);

  if(uv_is_closing(handle)){
    fprintf(stderr, "server:close() warning: server already closing. server: %p, handle: %p\n", server, handle);
    return 0;
  }

  uv_close(handle, links_tcp_server_onclose);
  return 0;
}

static int links_tcp_server_address(lua_State* L){
  if(lua_type(L, 1) != LUA_TUSERDATA){
    return luaL_argerror(L, 1, "server:address() error: server must be [userdata]\n"); 
  }

  links_tcp_server_t* server = lua_touserdata(L, 1);
  if(!server){
    return luaL_argerror(L, 1, "server:address() error: server is closed\n"); 
  }

  fprintf(stderr, "server_address: \n");
  return 0;
}

static int links_tcp_server_connections(lua_State* L){
  if(lua_type(L, 1) != LUA_TUSERDATA){
    return luaL_argerror(L, 1, "server:connections() error: server must be [userdata]\n"); 
  }

  links_tcp_server_t* server = lua_touserdata(L, 1);
  if(!server){
    return luaL_argerror(L, 1, "server:connections() error: server is closed\n"); 
  }
  
  lua_pushinteger(L, server->connections);
  return 1;
}

/*
static void links_tcp_socket_connect_timeout(uv_timer_t* handle){
  links_tcp_socket_t* socket = container_of(handle, links_tcp_socket_t, timer);
  lua_State* L = socket->thread;
  uv_timer_stop(&socket->timer);

  lua_pushnil(L);
  links_uv_error(L, UV_ETIMEDOUT);
  links_resume(L, 2, socket->self_ref, socket->thread_ref, "connect_timeout");
}
*/

/*static void links_tcp_socket_onconnect(uv_connect_t* req, int status){*/
/*}*/

static int links_tcp_socket_connect(lua_State* L){
  fprintf(stderr, "server_get_connections: \n");
  return 0;
}

static void links_tcp_socket_read_timeout(uv_timer_t* handle){
  links_tcp_socket_t* socket = container_of(handle, links_tcp_socket_t, timer);
  lua_State* L = socket->thread;

  uv_read_stop((uv_stream_t*)(&socket->handle));
  uv_timer_stop(&socket->timer);
  links_clear_flag(socket->flag, LINKS_READABLE_SHIFT);

  lua_pushnil(L);
  links_uv_error(L, UV_ETIMEDOUT);
  links_resume(L, 2, socket->self_ref, socket->thread_ref, "read_timeout");
}

static void links_tcp_socket_onalloc(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf){
  links_tcp_socket_t* socket = container_of(handle, links_tcp_socket_t, handle);
  if(!socket->read_buf){
    socket->read_buf = links_buf_alloc(socket->read_buf_size);
  }

  char* last;
  links_buf_t* read_buf = socket->read_buf;
  last = read_buf->last;
  buf->base = last;
  buf->len = read_buf->end - last;
}

static void links_tcp_socket_onread(uv_stream_t* handle, ssize_t nread, const uv_buf_t* buf){
  if(nread == 0){
    return;
  }
  
  links_tcp_socket_t* socket = container_of(handle, links_tcp_socket_t, handle);
  lua_State* L = socket->thread;
  links_buf_t* read_buf = socket->read_buf;
  size_t rest_size;

  if(nread > 0){
    size_t n = socket->bytes;

    socket->read_bytes += nread;
    read_buf->last += nread;
    rest_size = read_buf->last - read_buf->pos;
    
    if(n <= rest_size){
      lua_pushlstring(L, read_buf->pos, n);
      lua_pushnil(L);
      read_buf->pos += n;
      uv_read_stop((uv_stream_t*)(&socket->handle));
      uv_timer_stop(&socket->timer);
      links_resume(L, 2, socket->self_ref, socket->thread_ref, "read_ok");
    }
    return;
  }

  /*read error*/
  links_clear_flag(socket->flag, LINKS_READABLE_SHIFT);
  rest_size = read_buf->last - read_buf->pos;
  if(rest_size > 0){
    lua_pushlstring(L, read_buf->pos, rest_size);
  }else{
    lua_pushnil(L);
  }
  links_uv_error(L, nread);
  uv_read_stop((uv_stream_t*)(&socket->handle));
  uv_timer_stop(&socket->timer);
  links_resume(L, 2, socket->self_ref, socket->thread_ref, "read_error");
}

static int links_tcp_socket_read(lua_State* L){
  if(lua_type(L, 1) != LUA_TUSERDATA){
    return luaL_argerror(L, 1, "socket:read(n) error: socket must be [userdata]\n"); 
  }

  links_tcp_socket_t** socket_ptr = lua_touserdata(L, 1);
  links_tcp_socket_t* socket = NULL;
  if(!socket_ptr || !(socket = *socket_ptr)){
    return luaL_argerror(L, 1, "socket:read(n) error: socket is closed\n"); 
  }

  if(!links_check_flag(socket->flag, LINKS_READABLE_SHIFT)){
    if(links_check_flag(socket->flag, LINKS_CLOSE_SHIFT)){
      return luaL_error(L, "socket:read(n) error: unreadable[socket:close() has been called]\n"); 
    }
    return luaL_error(L, "socket:read(n) error: unreadable[please check the last socket:read(n) call error]\n"); 
  }
  
  int n;
  if(lua_type(L, 2) == LUA_TNUMBER){
    n = lua_tointeger(L, 2);
  }else{
    return luaL_argerror(L, 2, "socket:read(n) error: n is [required] and must be [number]\n"); 
  }

  if(n < 0){
    return luaL_argerror(L, 2, "socket:read(n) error: n must be >= 0\n"); 
  }

  if(n == 0){
    lua_pushliteral(L, "");
    lua_pushnil(L);
    return 2;
  }

  links_buf_t* read_buf = socket->read_buf;

  if(read_buf){
    int rest_size = read_buf->last - read_buf->pos;

    if(n > (int)read_buf->size){
      return luaL_error(L, "socket:read(n) error: n must be <= %d\n", read_buf->size); 
    }

    /**
      start    pos   n   last    end 
      |         |    |     |       |
      ----------++++++++++----------
     */
    if(n <= rest_size){
      lua_pushlstring(L, read_buf->pos, n);
      lua_pushnil(L);
      read_buf->pos += n;
      return 2;
    }

    /**
      start    pos      last     end      n
      |         |        |         |      |
      ----------++++++++++-----------------
     */
    if((read_buf->pos + n) > read_buf->end){
      links_memmove(read_buf->start, read_buf->pos, rest_size);
      read_buf->pos = read_buf->start;
      read_buf->last = read_buf->start + rest_size;
    }
  }

  if(socket->timeout){
    uv_timer_start(&socket->timer, links_tcp_socket_read_timeout, socket->timeout, 0);
  }

  socket->bytes = n;
  uv_read_start((uv_stream_t*)(&socket->handle), links_tcp_socket_onalloc, links_tcp_socket_onread);
  return lua_yield(L, 0);
}

static void links_tcp_socket_write_timeout(uv_timer_t* handle){
  links_tcp_socket_t* socket = container_of(handle, links_tcp_socket_t, timer);
  lua_State* L = socket->thread;

  uv_timer_stop(&socket->timer);
  luaL_unref(L, LUA_REGISTRYINDEX, socket->write_data_ref);
  links_clear_flag(socket->flag, LINKS_WRITEABLE_SHIFT);

  lua_pushinteger(L, 0);
  links_uv_error(L, UV_ETIMEDOUT);
  links_resume(L, 2, socket->self_ref, socket->thread_ref, "write_timeout");
}

static void links_tcp_socket_after_write(uv_write_t* req, int status){
  links_tcp_socket_t* socket = container_of(req, links_tcp_socket_t, req);
  lua_State* L = socket->thread;
 
  uv_timer_stop(&socket->timer);
  luaL_unref(L, LUA_REGISTRYINDEX, socket->write_data_ref);

  if(status < 0){
    links_clear_flag(socket->flag, LINKS_WRITEABLE_SHIFT);
    lua_pushinteger(L, 0);
    links_uv_error(L, status);
    links_resume(L, 2, socket->self_ref, socket->thread_ref, "write_error");
    return;
  }

  socket->write_bytes += socket->bytes;
  lua_pushinteger(L, socket->bytes);
  lua_pushnil(L);
  links_resume(L, 2, socket->self_ref, socket->thread_ref, "write_ok");
}

int links_tcp_socket_try_write(uv_stream_t* handle, uv_buf_t** bufs, size_t* count, size_t* written_bytes){
  uv_buf_t* vbufs = *bufs;
  size_t vcount = *count;

  int err = uv_try_write(handle, vbufs, vcount);
  if(err == UV_ENOSYS || err == UV_EAGAIN){
    return 0;
  }

  if(err < 0){
    return err;
  }

  *written_bytes = err;
  size_t written = err;
  for(; written != 0 && vcount > 0; vbufs++, vcount--){
    if(vbufs[0].len > written){
      vbufs[0].base += written;
      vbufs[0].len -= written;
      written = 0;
      break;
    }else{
      written -= vbufs[0].len;
    }
  }

  *bufs = vbufs;
  *count = vcount;

  return 0;
}

static int links_tcp_socket_write(lua_State* L){
  if(lua_type(L, 1) != LUA_TUSERDATA){
    return luaL_argerror(L, 1, "socket:write(data) error: socket must be [userdata]\n"); 
  }

  links_tcp_socket_t** socket_ptr = lua_touserdata(L, 1);
  links_tcp_socket_t* socket = NULL;
  if(!socket_ptr || !(socket = *socket_ptr)){
    return luaL_argerror(L, 1, "socket:write(data) error: socket is closed\n"); 
  }

  if(!links_check_flag(socket->flag, LINKS_WRITEABLE_SHIFT)){
    if(links_check_flag(socket->flag, LINKS_CLOSE_SHIFT)){
      return luaL_error(L, "socket:write(data) error: unwriteable[socket:close() has been called]\n"); 
    }
    if(links_check_flag(socket->flag, LINKS_SHUTDOWN_SHIFT)){
      return luaL_error(L, "socket:write(data) error: unwriteable[socket:shutdown() has been called]\n"); 
    }
    return luaL_error(L, "socket:write(data) error: unwriteable[please check the last socket:write(data) call error]\n"); 
  }

  uv_buf_t* bufs;
  size_t count;
  size_t len;
  if(lua_type(L, 2) == LUA_TTABLE){
    count = lua_rawlen(L, 2);
    bufs = links_palloc(sizeof(uv_buf_t) * count);
    if(!bufs){
      return luaL_error(L, "socket:write(data) links_pmalloc(sizeof(uv_buf_t) * %" PRId64 ") error: out of memory \n", count); 
    }

    for(size_t i = 0; i < count; ++i){
      lua_rawgeti(L, 2, i + 1);
      bufs[i].base = (char*) luaL_checklstring(L, -1, &len);
      bufs[i].len = len;
      lua_pop(L, 1);
    }
  }else if(lua_type(L, 2) == LUA_TSTRING){
    count = 1;
    bufs = links_palloc(sizeof(uv_buf_t) * count);
    if(!bufs){
      return luaL_error(L, "socket:write(data) links_pmalloc(sizeof(uv_buf_t) * %" PRId64 ") error: out of memory \n", count); 
    }

    bufs[0].base = (char*) luaL_checklstring(L, -1, &len);
    bufs[0].len = len;
  }else{
    return luaL_argerror(L, 2, "socket:write(data) error: data must be table[string array] or string\n"); 
  }

  void* tmp = bufs;

  size_t written = 0;
  size_t vcount = count;
  uv_stream_t* handle = (uv_stream_t*)(&socket->handle);
  int err = links_tcp_socket_try_write(handle, &bufs, &vcount, &written);
  if(err){
    links_pfree(tmp);
    lua_pushinteger(L, 0);
    links_uv_error(L, err);
    return 2;
  }

  if(vcount == 0){
    links_pfree(tmp);
    lua_pushinteger(L, written);
    lua_pushnil(L);
    return 2;
  }

  size_t bytes = 0;
  for(size_t i = 0; i < vcount; i++){
    bytes += bufs[i].len;
  }
  
  uv_write_t* req = &(socket->req.write_req);
  err = uv_write2(req, handle, bufs, vcount, NULL, links_tcp_socket_after_write);
  if(err){
    links_pfree(tmp);
    lua_pushinteger(L, written);
    links_uv_error(L, err);
    return 2;
  }

  if(socket->timeout){
    uv_timer_start(&socket->timer, links_tcp_socket_write_timeout, socket->timeout, 0);
  }

  lua_pushvalue(L, 2);
  socket->write_data_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  socket->bytes = bytes;
  links_pfree(tmp);
  return lua_yield(L, 0);
}

static void links_tcp_socket_after_shutdown(uv_shutdown_t* req, int status){
  links_tcp_socket_t* socket = container_of(req, links_tcp_socket_t, req);
  lua_State* L = socket->thread;

  /*luaL_unref(L, LUA_REGISTRYINDEX, socket->write_data_ref);*/
  /*uv_timer_stop(&socket->timer); */
  if(status < 0){
    links_uv_error(L, status);
    links_resume(L, 1, socket->self_ref, socket->thread_ref, "shutdown_error");
  }

  lua_pushnil(L);
  links_resume(L, 1, socket->self_ref, socket->thread_ref, "shutdown_ok");
}

static int links_tcp_socket_shutdown(lua_State* L){
  if(lua_type(L, 1) != LUA_TUSERDATA){
    return luaL_argerror(L, 1, "socket:shutdown() error: socket must be [userdata]\n"); 
  }

  links_tcp_socket_t** socket_ptr = lua_touserdata(L, 1);
  links_tcp_socket_t* socket = NULL;
  if(!socket_ptr || !(socket = *socket_ptr)){
    return luaL_error(L, "socket:shutdown() error: socket is closed\n"); 
  }
  
  if(links_check_flag(socket->flag, LINKS_SHUTDOWN_SHIFT)){
    if(links_check_flag(socket->flag, LINKS_SHUTDOWN_SHIFT)){
      return luaL_error(L, "socket:shutdown() error: socket:close() has been called\n");
    }
    return luaL_error(L, "socket:shutdown() error: socket:shutdown() has been called\n");
  }

  links_clear_flag(socket->flag, LINKS_WRITEABLE_SHIFT);
  links_set_flag(socket->flag, LINKS_SHUTDOWN_SHIFT);

  uv_shutdown_t* req = &(socket->req.shutdown_req);
  uv_stream_t* handle = (uv_stream_t*)(&socket->handle);
  int err = uv_shutdown(req, handle, links_tcp_socket_after_shutdown);
  if(err){
    links_uv_error(L, err);
    return 1;
  }
  return lua_yield(L, 0);
}

static void links_tcp_socket_onclose(uv_handle_t* handle){
  links_tcp_socket_t* socket = container_of(handle, links_tcp_socket_t, handle);
  /*lua_State* L = links_get_main_thread();*/
  lua_State* L = socket->thread;
  /*uv_timer_stop(&socket->timer); */

  /*luaL_unref(L, LUA_REGISTRYINDEX, socket->write_data_ref);*/
  links_resume(L, 0, socket->self_ref, socket->thread_ref, "closed");
  links_free(socket);
}

static int links_tcp_socket_close(lua_State* L){
  if(lua_type(L, 1) != LUA_TUSERDATA){
    return luaL_argerror(L, 1, "socket:close() error: socket must be [userdata]\n"); 
  }

  links_tcp_socket_t** socket_ptr = lua_touserdata(L, 1);
  links_tcp_socket_t* socket = NULL;
  if(!socket_ptr || !(socket = *socket_ptr)){
    return luaL_error(L, "socket:close() error: socket is closed\n"); 
  }
  
  if(links_check_flag(socket->flag, LINKS_CLOSE_SHIFT)){
    return luaL_error(L, "socket:close() error: socket:close() has been called\n"); 
  }

  links_clear_flag(socket->flag, LINKS_READABLE_SHIFT);
  links_clear_flag(socket->flag, LINKS_WRITEABLE_SHIFT);
  links_set_flag(socket->flag, LINKS_CLOSE_SHIFT);

  if(socket->server){
    socket->server->connections--;
  }
  
  links_buf_free(socket->read_buf);

  uv_handle_t* handle = (uv_handle_t*)(&socket->handle);
  uv_close(handle, links_tcp_socket_onclose);
  return lua_yield(L, 0);
}

int luaopen_tcp(lua_State *L){
  /*server metatable*/
  lua_pushlightuserdata(L, &links_tcp_server_metatable_key);
  lua_createtable(L, 0, 3);
  lua_pushcfunction(L, links_tcp_server_close);
  lua_setfield(L, -2, "close");

  lua_pushcfunction(L, links_tcp_server_address);
  lua_setfield(L, -2, "address");

  lua_pushcfunction(L, links_tcp_server_connections);
  lua_setfield(L, -2, "connections");

  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_rawset(L, LUA_REGISTRYINDEX);

  /*socket metatable*/
  lua_pushlightuserdata(L, &links_tcp_socket_metatable_key);
  lua_createtable(L, 0, 3);
  lua_pushcfunction(L, links_tcp_socket_read);
  lua_setfield(L, -2, "read");

  lua_pushcfunction(L, links_tcp_socket_write);
  lua_setfield(L, -2, "write");

  lua_pushcfunction(L, links_tcp_socket_shutdown);
  lua_setfield(L, -2, "shutdown");

  lua_pushcfunction(L, links_tcp_socket_close);
  lua_setfield(L, -2, "close");

  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  lua_rawset(L, LUA_REGISTRYINDEX);

  luaL_Reg lib[] = {
    { "createServer", links_tcp_create_server},
    { "connect", links_tcp_socket_connect},
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
