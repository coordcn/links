/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "alloc.h"
#include "hash.h"
#include "uv.h"

static links_pool_t links_hash_item_pool;
static links_pool_t* links_hash_item_pool_ptr;

/*double slots size and rehash*/
static void links_hash_rehash(links_hash_t* hash){
  size_t old_size = 1 << hash->bits;
  size_t bits = hash->bits++;
  size_t size = 1 << bits;
  hash->max_items = size;
  links_hlist_head_t* old_slots = hash->slots;
  links_hlist_head_t* new_slots = (links_hlist_head_t*)links_malloc(sizeof(links_hlist_head_t) * size);
  for(size_t i = 0; i < size; i++){
    new_slots[i].first = NULL;
  }

  links_hlist_head_t* current_slot;
  links_hlist_node_t* pos;
  links_hash_item_t* item;
  size_t index;
  for(size_t i = 0; i < old_size; i++){
    current_slot = &old_slots[i];
    while(!links_hlist_is_empty(current_slot)){
      pos = current_slot->first;
      links_hlist_remove(pos);
      item = (links_hash_item_t*)links_hlist_entry(pos, links_hash_item_t, node);
      index = links_hash_slot(item->hash, bits);
      links_hlist_insert_head(&item->node, &new_slots[index]);
    }
  }

  hash->slots = new_slots;
  links_free(old_slots);
}

links_hash_t* links_hash__create(size_t bits, 
                                 size_t max_age, 
                                 links_hash_free_fn free_fn, 
                                 links_hash_strcmp_fn strcmp_fn, 
                                 links_hash_fn hash_fn){
  if(!links_hash_item_pool_ptr){
    links_pool_init(&links_hash_item_pool, LINKS_HASH_ITEM_POOL_MAX_FREE_CHUNKS);
    links_hash_item_pool_ptr = &links_hash_item_pool;
  }

  links_hash_t* hash = (links_hash_t*)links_malloc(sizeof(links_hash_t));
  hash->bits = bits;
  size_t size = 1 << bits;
  hash->max_items = size;
  hash->max_age = max_age;
  hash->free = free_fn;
  hash->str_cmp = strcmp_fn;
  hash->hash = hash_fn;
  hash->items = 0;
  links_hlist_head_t* slots = (links_hlist_head_t*)links_malloc(sizeof(links_hlist_head_t) * size);
  for(size_t i = 0; i < size; i++){
    slots[i].first = NULL;
  }
  hash->slots = slots;

  return hash;
}

void links_hash_destroy(links_hash_t* hash){
  size_t size = 1 << hash->bits;
  links_hlist_head_t* slots = hash->slots;

  links_hlist_head_t* current_slot;
  links_hlist_node_t* pos;
  links_hash_item_t* item;
  for(size_t i = 0; i < size; i++){
    current_slot = &slots[i];
    while(!links_hlist_is_empty(current_slot)){
      pos = current_slot->first;
      links_hlist_remove(pos);
      item = (links_hash_item_t*)links_hlist_entry(pos, links_hash_item_t, node);
      if(item->pointer) hash->free(item->pointer); 
      if(item->key) links_free(item->key);
      links_pfree(&links_hash_item_pool, item);
    }
  }

  links_free(slots);
  links_free(hash);
}

void links_hash_str_set(links_hash_t* hash, char* key, size_t n, void* pointer){
  if(hash->items >= hash->max_items){
    links_hash_rehash(hash);
  }

  size_t hash_key = hash->hash(key, n);
  size_t index = links_hash_slot(hash_key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;
  int cmp;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_hlist_entry(pos, links_hash_item_t, node);
      if(item->key_length == n){
        cmp = hash->str_cmp(item->key, key, n);
        if(!cmp){
          hash->free(item->pointer);
          item->pointer = pointer;
          if(hash->max_age){
            item->expires = uv_now(uv_default_loop()) + hash->max_age;
          }else{
            item->expires = 0;
          }
          return;
        }
      }
    }
  }

  item = (links_hash_item_t*)links_palloc(&links_hash_item_pool, sizeof(links_hash_item_t));
  item->pointer = pointer;
  item->hash = hash_key;
  if(hash->max_age){
    item->expires = uv_now(uv_default_loop()) + hash->max_age;
  }else{
    item->expires = 0;
  }
  item->key_length = n;
  item->key = key;
  links_hlist_insert_head(&item->node, slot);
  hash->items++;
}

void* links_hash_str_get(links_hash_t* hash, const char* key, size_t n){
  size_t hash_key = hash->hash(key, n);
  size_t index = links_hash_slot(hash_key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;
  int cmp;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->key_length == n){
        cmp = hash->str_cmp(item->key, key, n);
        if(!cmp){
          if(item->expires && item->expires < uv_now(uv_default_loop())){
            return NULL;
          }
          return item->pointer;
        }
      }
    }
  }

  return NULL;
}

void links_hash_str_remove(links_hash_t* hash, char* key, size_t n){
  size_t hash_key = hash->hash(key, n);
  size_t index = links_hash_slot(hash_key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;
  int cmp;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->key_length == n){
        cmp = hash->str_cmp(item->key, key, n);
        if(!cmp){
          links_hlist_remove(&item->node);
          hash->free(item->pointer);
          links_free(item->key);
          links_pfree(&links_hash_item_pool, item);
          hash->items--;
          return;
        }
      }
    }
  }
}

void links_hash_int_set(links_hash_t* hash, size_t key, void* pointer){
  if(hash->items >= hash->max_items){
    links_hash_rehash(hash);
  }

  size_t index = links_hash_slot(key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->hash == key){
        hash->free(item->pointer);
        item->pointer = pointer;
        if(hash->max_age){
          item->expires = uv_now(uv_default_loop()) + hash->max_age;
        }else{
          item->expires = 0;
        }
        return;
      }
    }
  }

  item = (links_hash_item_t*)links_palloc(&links_hash_item_pool, sizeof(links_hash_item_t));
  item->pointer = pointer;
  item->hash = key;
  if(hash->max_age){
    item->expires = uv_now(uv_default_loop()) + hash->max_age;
  }else{
    item->expires = 0;
  }
  links_hlist_insert_head(&item->node, slot);
  hash->items++;
}

void* links_hash_int_get(links_hash_t* hash, size_t key){
  size_t index = links_hash_slot(key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->hash == key){
        if(item->expires && item->expires < uv_now(uv_default_loop())){
          return NULL;
        }
        return item->pointer;
      }
    }
  }

  return NULL;
}

void links_hash_int_remove(links_hash_t* hash, size_t key){
  size_t index = links_hash_slot(key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->hash == key){
        links_hlist_remove(&item->node);
        hash->free(item->pointer);
        links_pfree(&links_hash_item_pool, item);
        hash->items--;
        return;
      }
    }
  }
}

void links_hash_int_set_int32(links_hash_t* hash, size_t key, int32_t value){
  if(hash->items >= hash->max_items){
    links_hash_rehash(hash);
  }

  size_t index = links_hash_slot(key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->hash == key){
        item->value.int32 = value;
        if(hash->max_age){
          item->expires = uv_now(uv_default_loop()) + hash->max_age;
        }else{
          item->expires = 0;
        }
        return;
      }
    }
  }

  item = (links_hash_item_t*)links_palloc(&links_hash_item_pool, sizeof(links_hash_item_t));
  item->value.int32 = value;
  item->hash = key;
  if(hash->max_age){
    item->expires = uv_now(uv_default_loop()) + hash->max_age;
  }else{
    item->expires = 0;
  }
  links_hlist_insert_head(&item->node, slot);
  hash->items++;
}

int32_t links_hash_int_get_int32(links_hash_t* hash, size_t key){
  size_t index = links_hash_slot(key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->hash == key){
        if(item->expires && item->expires < uv_now(uv_default_loop())){
          return NULL;
        }
        return item->value.int32;
      }
    }
  }

  return NULL;
}

void links_hash_int_remove_int32(links_hash_t* hash, size_t key){
  size_t index = links_hash_slot(key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->hash == key){
        links_hlist_remove(&item->node);
        links_pfree(&links_hash_item_pool, item);
        hash->items--;
        return;
      }
    }
  }
}
