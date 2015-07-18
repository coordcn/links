/**************************************************************
  @author: QianYe(coordcn@163.com)
  @reference: luvit/luv_dns.c
              node.js/cares_wrap.cc
 **************************************************************/

#include "hash.h"
#include "init.h"
#include "links.h"
#include "ares.h"

static ares_channel links_ares_channel;
static uv_timer_t links_ares_timer;
static links_pool_t links_ares_task_pool;
static links_pool_t links_dns_arg_pool;
static links_hash_t* links_ares_tasks;
static links_hash_t* links_dns_ipv4_cache;
static links_hash_t* links_dns_ipv6_cache;

#define DNS_MAX_AGE_DEFAULT 7 * 24 * 3600 * 1000

typedef struct {
  UV_HANDLE_FIELDS
  ares_socket_t sock;
  uv_poll_t poll_watcher;
} links_ares_task_t;

typedef struct {
  struct hostent* ipv4_host;
  uint64_t ipv4_expires;
  struct hostent* ipv6_host;
  uint64_t ipv6_expires;
} links_dns_result_t;

typedef struct {
  lua_State* L;
  size_t len;
  char* name; 
} links_dns_arg_t;

static void links_ares_timeout(uv_timer_t* handle){
  ares_process_fd(links_ares_channel, ARES_SOCKET_BAD, ARES_SOCKET_BAD);
}

static void links_ares_poll_callback(uv_poll_t* watcher, int status, int events){
  links_ares_task_t* task = container_of(watcher, links_ares_task_t, poll_watcher);

  /* Reset the idle timer */
  uv_timer_again(&links_ares_timer);

  /* An error happened. Just pretend that the socket is both readable and writable*/
  if(status < 0){
    ares_process_fd(links_ares_channel, task->sock, task->sock);
    return;
  }

  /* Process DNS responses */
  ares_process_fd(links_ares_channel,
                  events & UV_READABLE ? task->sock : ARES_SOCKET_BAD,
                  events & UV_WRITABLE ? task->sock : ARES_SOCKET_BAD);
}

static void links_ares_poll_close_callback(uv_handle_t* watcher){
  links_ares_task_t* task = container_of(watcher, links_ares_task_t, poll_watcher);
  links_hash_int_remove(links_ares_tasks, task->sock);

  if(!links_ares_tasks->items){
    uv_timer_stop(&links_ares_timer);
  }
}

static links_ares_task_t* links_ares_task_create(uv_loop_t* loop, ares_socket_t sock){
  links_ares_task_t* task = (links_ares_task_t*)links_palloc(&links_ares_task_pool,  
                                                             sizeof(links_ares_task_t));

  if(!task){
    /* Out of memory. */
    return NULL;
  }

  task->loop = loop;
  task->sock = sock;

  if(uv_poll_init_socket(loop, &task->poll_watcher, sock) < 0){
    /* This should never happen. */
    links_free(task);
    return NULL;
  }

  return task;
}

static void links_ares_sockstate_callback(void* data,
                                          ares_socket_t sock,
                                          int read,
                                          int write){
  links_ares_task_t* task;

  task = (links_ares_task_t*)links_hash_int_get(links_ares_tasks, sock);

  if(read || write){
    if(!task){
      /* New socket */

      /* If this is the first socket then start the timer. */
      if(!uv_is_active((uv_handle_t*)(&links_ares_timer))){
        uv_timer_start(&links_ares_timer, links_ares_timeout, 1000, 1000);
      }

      task = links_ares_task_create(uv_default_loop(), sock);
      if(!task){
        /* This should never happen unless we're out of memory or something */
        /* is seriously wrong. The socket won't be polled, but the the query */
        /* will eventually time out. */
        return;
      }

      links_hash_int_set(links_ares_tasks, sock, task);
    }
    
     /* This should never fail. If it fails anyway, the query will eventually time out. */
    uv_poll_start(&task->poll_watcher,
                  (read ? UV_READABLE : 0) | (write ? UV_WRITABLE : 0),
                  links_ares_poll_callback);
  }else{
    /* read == 0 and write == 0 this is c-ares's way of notifying us that */
    /* the socket is now closed. We must free the data associated with socket. */
    uv_close((uv_handle_t*)(&task->poll_watcher), links_ares_poll_close_callback);
  }
}

static void links_ares_free_task(void *p){
  links_pfree(&links_ares_task_pool, p);
}

static void links_ares_free_hostent(void *p){
  ares_free_hostent((struct hostent*)p);
}

/* From NodeJS */
static const char* links_ares_errno_string(int errorno)
{
  switch (errorno) {
    #define ERRNO_CASE(e) case e: return #e;
    ERRNO_CASE(ARES_SUCCESS)
    ERRNO_CASE(ARES_ENODATA)
    ERRNO_CASE(ARES_EFORMERR)
    ERRNO_CASE(ARES_ESERVFAIL)
    ERRNO_CASE(ARES_ENOTFOUND)
    ERRNO_CASE(ARES_ENOTIMP)
    ERRNO_CASE(ARES_EREFUSED)
    ERRNO_CASE(ARES_EBADQUERY)
    ERRNO_CASE(ARES_EBADNAME)
    ERRNO_CASE(ARES_EBADFAMILY)
    ERRNO_CASE(ARES_EBADRESP)
    ERRNO_CASE(ARES_ECONNREFUSED)
    ERRNO_CASE(ARES_ETIMEOUT)
    ERRNO_CASE(ARES_EOF)
    ERRNO_CASE(ARES_EFILE)
    ERRNO_CASE(ARES_ENOMEM)
    ERRNO_CASE(ARES_EDESTRUCTION)
    ERRNO_CASE(ARES_EBADSTR)
    ERRNO_CASE(ARES_EBADFLAGS)
    ERRNO_CASE(ARES_ENONAME)
    ERRNO_CASE(ARES_EBADHINTS)
    ERRNO_CASE(ARES_ENOTINITIALIZED)
    ERRNO_CASE(ARES_ELOADIPHLPAPI)
    ERRNO_CASE(ARES_EADDRGETNETWORKPARAMS)
    ERRNO_CASE(ARES_ECANCELLED)
    #undef ERRNO_CASE
  default:
    return "ARES_UNKNOWN";
  }
}

void links_dns_init(){
  uv_loop_t* loop = uv_default_loop();
  int ret = ares_library_init(ARES_LIB_INIT_ALL);
  if(ret != ARES_SUCCESS){
    fprintf(stderr, "ares_library_init() failed, Error: %s.\n", links_ares_errno_string(ret));
  }

  struct ares_options options;
  links_memzero(&options, sizeof(options));
  options.flags = ARES_FLAG_NOCHECKRESP;
  options.sock_state_cb = links_ares_sockstate_callback;
  options.sock_state_cb_data = loop;

  ret = ares_init_options(&links_ares_channel, 
                          &options, 
                          ARES_OPT_FLAGS | ARES_OPT_SOCK_STATE_CB);
  if(ret != ARES_SUCCESS){
    fprintf(stderr, "ares_init_options() failed, Error: %s.\n", links_ares_errno_string(ret));
  }

  uv_timer_init(loop, &links_ares_timer);
  links_pool_init(&links_ares_task_pool, 1024);
  links_pool_init(&links_dns_arg_pool, 1024);
  links_ares_tasks = links_hash_create_int_pointer(LINKS_2K_SHIFT,
                                       0,
                                       links_ares_free_task);
  links_dns_ipv4_cache = links_hash_create_str_pointer(LINKS_8K_SHIFT, 
                                         DNS_MAX_AGE_DEFAULT, 
                                         links_ares_free_hostent);
  links_dns_ipv6_cache = links_hash_create_str_pointer(LINKS_8K_SHIFT, 
                                         DNS_MAX_AGE_DEFAULT, 
                                         links_ares_free_hostent);
}

static void links_dns_host2addrs(lua_State* L, struct hostent* host){
  char ip[INET6_ADDRSTRLEN];

  lua_newtable(L);
  for(int i=0; host->h_addr_list[i]; ++i){
    uv_inet_ntop(host->h_addrtype, host->h_addr_list[i], ip, sizeof(ip));
    lua_pushstring(L, ip);
    lua_rawseti(L, -2, i + 1);
  }
}

static void links_dns_error(lua_State* L, int errorno){
  const char* err = links_ares_errno_string(errorno);

  lua_createtable(L, 0, 3);
  lua_pushinteger(L, errorno);
  lua_setfield(L, -2, "code");
  lua_pushstring(L, err);
  lua_setfield(L, -2, "name");
  lua_pushstring(L, err);
  lua_setfield(L, -2, "message");
}

static void links_dns_queryA_callback(void* data, 
                                      int status, 
                                      int timeouts, 
                                      unsigned char* buf,
                                      int len){
  links_dns_arg_t* arg = (links_dns_arg_t*)data;
  lua_State* L = arg->L;

  if(status != ARES_SUCCESS){
    lua_pushnil(L);
    links_dns_error(L, status);
    lua_resume(L, NULL, 2);
    links_pfree(&links_dns_arg_pool, arg);
    return;
  }

  struct hostent* host;
  int rc = ares_parse_a_reply(buf, len, &host, NULL, NULL);
  if(rc != ARES_SUCCESS){
    lua_pushnil(L);
    links_dns_error(L, rc);
    lua_resume(L, NULL, 2);
    links_pfree(&links_dns_arg_pool, arg);
    return;
  }

  links_hash_str_set(links_dns_ipv4_cache, arg->name, arg->len, host);
  links_dns_host2addrs(L, host);
  lua_pushnil(L);
  lua_resume(L, NULL, 2);
  links_pfree(&links_dns_arg_pool, arg);
}

static int links_dns_queryA(lua_State* L){
  if(L == links_get_main_thread()){
    return luaL_error(L, "dns.resolve4(name) error: must be used in a coroutine"); 
  }

  if(lua_type(L, 1) != LUA_TSTRING){
    return luaL_argerror(L, 1, "dns.resolve4(name) error: name must be [string]"); 
  }

  size_t n;
  const char* name = luaL_tolstring(L, 1, &n);

  struct hostent* host = (struct hostent*)links_hash_str_get(links_dns_ipv4_cache, name, n);
  if(host){
    links_dns_host2addrs(L, host);
    lua_pushnil(L);
    return 2;
  }
  
  links_dns_arg_t* arg = links_palloc(&links_dns_arg_pool, sizeof(links_dns_arg_t));
  arg->L = L;
  arg->len = n;
  arg->name = links_strndup(name, n);

  ares_query(links_ares_channel,
             name,
             ns_c_in,
             ns_t_a,
             links_dns_queryA_callback,
             arg);

  return lua_yield(L, 0);
}

static void links_dns_queryAaaa_callback(void* data, 
                                         int status, 
                                         int timeouts, 
                                         unsigned char* buf,
                                         int len){
  links_dns_arg_t* arg = (links_dns_arg_t*)data;
  lua_State* L = arg->L;

  if(status != ARES_SUCCESS){
    lua_pushnil(L);
    links_dns_error(L, status);
    lua_resume(L, NULL, 2);
    links_free(arg);
    return;
  }

  struct hostent* host;
  int rc = ares_parse_aaaa_reply(buf, len, &host, NULL, NULL);
  if(rc != ARES_SUCCESS){
    lua_pushnil(L);
    links_dns_error(L, rc);
    lua_resume(L, NULL, 2);
    links_free(arg);
    return;
  }

  links_hash_str_set(links_dns_ipv6_cache, arg->name, arg->len, host);
  links_dns_host2addrs(L, host);
  lua_pushnil(L);
  lua_resume(L, NULL, 2);
  links_free(arg);
}

static int links_dns_queryAaaa(lua_State* L){
  if(L == links_get_main_thread()){
    return luaL_error(L, "dns.resolve6(name) error: must be used in a coroutine"); 
  }

  if(lua_type(L, 1) != LUA_TSTRING){
    return luaL_argerror(L, 1, "dns.resolve6(name) error: name must be [string]"); 
  }

  size_t n;
  const char* name = luaL_tolstring(L, 1, &n);

  struct hostent* host = (struct hostent*)links_hash_str_get(links_dns_ipv6_cache, name, n);
  if(host){
    links_dns_host2addrs(L, host);
    lua_pushnil(L);
    return 2;
  }
  
  links_dns_arg_t* arg = links_malloc(sizeof(links_dns_arg_t));
  arg->L = L;
  arg->len = n;
  arg->name = links_strndup(name, n);

  ares_query(links_ares_channel,
             name,
             ns_c_in,
             ns_t_aaaa,
             links_dns_queryAaaa_callback,
             arg);

  return lua_yield(L, 0);
}

int luaopen_dns(lua_State *L){
  luaL_Reg lib[] = {
    { "resolve4", links_dns_queryA },
    { "resolve6", links_dns_queryAaaa },
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
