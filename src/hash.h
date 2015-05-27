/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#ifndef LINKS_HASH_H
#define LINKS_HASH_H

#include "list.h"
#include "alloc.h"
#include "config.h"

#if LINKS_BITS == 16
#define links_hash_slot(value, bits) links_hash_slot16(value, bits)
#elif LINKS_BITS == 32
#define links_hash_slot(value, bits) links_hash_slot32(value, bits)
#elif LINKS_BITS == 64
#define links_hash_slot(value, bits) links_hash_slot64(value, bits)
#else
#error LINKS_BITS not 16 or 32 or 64
#endif

#define links_hash(str, n) links_hash_DJB(str, n)
#define links_hash_lower(str, n) links_hash_DJB_lower(str, n)

static inline size_t links_hash_DJB(const char* str, size_t n){
  size_t hash = 5381;

  for(size_t i = 0; i < n; i++){
    hash += (hash << 5) + str[i];
  }

  return hash;
}

static inline size_t links_hash_DJB_lower(const char* str, size_t n){
  size_t hash = 5381;
  
  for(size_t i = 0; i < n; i++){
    hash += (hash << 5) + links_lower(str[i]);
  }

  return hash;
}

#define GOLDEN_RATIO_PRIME_16       40503UL
#define GOLDEN_RATIO_PRIME_32       2654435769UL
#define GOLDEN_RATIO_PRIME_64       11400714819323198485UL

/**
  if slot size is 1024, used_bits = 10
  bits = 64 - used_bits;
 */
static inline size_t links_hash_slot16(size_t value, size_t bits){
  return (value * GOLDEN_RATIO_PRIME_16) >> (16 - bits);
}

/**
  if slot size is 1024, used_bits = 10
  bits = 64 - used_bits;
 */
static inline size_t links_hash_slot32(size_t value, size_t bits){
  return (value * GOLDEN_RATIO_PRIME_32) >> (32 - bits);
}

/**
  if slot size is 1024, used_bits = 10
  bits = 64 - used_bits;
  linux kernel hash.h is better
 */
static inline size_t links_hash_slot64(size_t value, size_t bits){
  return (value * GOLDEN_RATIO_PRIME_64) >> (64 - bits);
}

static inline size_t links_hash_slot_ptr(const void* ptr, size_t bits){
  return links_hash_slot((size_t)ptr, bits);
}

typedef void (*links_hash_free_value)(void* p);

typedef struct {
  size_t max_items;
  size_t max_age;
  size_t items;
  size_t bits;
  links_hash_free_value free_value;
  links_hlist_head_t* slots;
} links_hash_t;

typedef struct {
  links_hlist_node_t node;
  void* value;
  size_t hash;
  size_t expires;
  size_t key_length;
  char* key;
} links_hash_item_t;

links_hash_t* links_hash_create(size_t bits, size_t max_age, links_hash_free_value free_value);
void links_hash_destroy(links_hash_t* hash);

void links_hash_str_set(links_hash_t* hash, char* key, size_t n, void* value);
void* links_hash_str_get(links_hash_t* hash, const char* key, size_t n);
void links_hash_str_remove(links_hash_t* hash, char* key, size_t n);

void links_hash_strlower_set(links_hash_t* hash, char* key, size_t n, void* value);
void* links_hash_strlower_get(links_hash_t* hash, const char* key, size_t n);
void links_hash_strlower_remove(links_hash_t* hash, char* key, size_t n);

void links_hash_int_set(links_hash_t* hash, size_t key, void* value);
void* links_hash_int_get(links_hash_t* hash, size_t key);
void links_hash_int_remove(links_hash_t* hash, size_t key);

#endif /*LINKS_HASH_H*/
