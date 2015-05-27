/**************************************************************
  @copyright: Copyright(c) 2015 coord.cn All rights reserved
  @author: QianYe(coordcn@163.com)
  @license: MIT license
  @overview: 
 **************************************************************/

#include "alloc.h"
#include "hash.h"
#include "uv.h"

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

  links_hlist_node_t* pos;
  links_hash_item_t* item;
  size_t index;
  for(size_t i = 0; i < old_size; i++){
    if(!links_hlist_is_empty(&old_slots[i])){
      links_hlist_for_each(pos, &old_slots[i]){
        item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
        index = links_hash_slot(item->hash, bits);
        links_hlist_insert_head(&item->node, &new_slots[index]);
      }
    }
  }

  hash->slots = new_slots;
  links_free(old_slots);
}

links_hash_t* links_hash_create(size_t bits, size_t max_age, links_hash_free_value free_value){
  links_hash_t* hash = (links_hash_t*)links_malloc(sizeof(links_hash_t));
  hash->bits = bits;
  size_t size = 1 << bits;
  hash->max_items = size;
  hash->max_age = max_age;
  hash->free_value = free_value;
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

  links_hlist_node_t* pos;
  links_hash_item_t* item;
  for(size_t i = 0; i < size; i++){
    if(!links_hlist_is_empty(&slots[i])){
      links_hlist_for_each(pos, &slots[i]){
        item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
        hash->free_value(item->value);
        links_free(item->key);
        links_free(item);
      }
    }
  }

  links_free(slots);
  links_free(hash);
}

void links_hash_str_set(links_hash_t* hash, char* key, size_t n, void* value){
  if(hash->items >= hash->max_items){
    links_hash_rehash(hash);
  }

  size_t hash_key = links_hash(key, n);
  size_t index = links_hash_slot(hash_key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;
  int cmp;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->key_length == n){
        cmp = links_memcmp(item->key, key, n);
        if(!cmp){
          hash->free_value(item->value);
          item->value = value;
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

  item = (links_hash_item_t*)links_malloc(sizeof(links_hash_item_t));
  item->value = value;
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
  size_t hash_key = links_hash(key, n);
  size_t index = links_hash_slot(hash_key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;
  int cmp;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->key_length == n){
        cmp = links_memcmp(item->key, key, n);
        if(!cmp){
          if(item->expires && item->expires < uv_now(uv_default_loop())){
            return NULL;
          }
          return item->value;
        }
      }
    }
  }

  return NULL;
}

void links_hash_str_remove(links_hash_t* hash, char* key, size_t n){
  size_t hash_key = links_hash(key, n);
  size_t index = links_hash_slot(hash_key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;
  int cmp;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->key_length == n){
        cmp = links_memcmp(item->key, key, n);
        if(!cmp){
          links_hlist_remove(&item->node);
          hash->free_value(item->value);
          links_free(item->key);
          links_free(item);
          hash->items--;
          return;
        }
      }
    }
  }
}

void links_hash_strlower_set(links_hash_t* hash, char* key, size_t n, void* value){
  if(hash->items >= hash->max_items){
    links_hash_rehash(hash);
  }

  size_t hash_key = links_hash_lower(key, n);
  size_t index = links_hash_slot(hash_key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;
  int cmp;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->key_length == n){
        cmp = links_strncasecmp(item->key, key, n);
        if(!cmp){
          hash->free_value(item->value);
          item->value = value;
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

  item = (links_hash_item_t*)links_malloc(sizeof(links_hash_item_t));
  item->value = value;
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

void* links_hash_strlower_get(links_hash_t* hash, const char* key, size_t n){
  size_t hash_key = links_hash(key, n);
  size_t index = links_hash_slot(hash_key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;
  int cmp;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->key_length == n){
        cmp = links_strncasecmp(item->key, key, n);
        if(!cmp){
          if(item->expires && item->expires < uv_now(uv_default_loop())){
            return NULL;
          }
          return item->value;
        }
      }
    }
  }

  return NULL;
}

void links_hash_strlower_remove(links_hash_t* hash, char* key, size_t n){
  size_t hash_key = links_hash_lower(key, n);
  size_t index = links_hash_slot(hash_key, hash->bits);
  links_hlist_head_t* slot = &hash->slots[index];
  links_hlist_node_t* pos;
  links_hash_item_t* item;
  int cmp;

  if(!links_hlist_is_empty(slot)){
    links_hlist_for_each(pos, slot){
      item = (links_hash_item_t*)links_list_entry(pos, links_hash_item_t, node);
      if(item->key_length == n){
        cmp = links_strncasecmp(item->key, key, n);
        if(!cmp){
          links_hlist_remove(&item->node);
          hash->free_value(item->value);
          links_free(item->key);
          links_free(item);
          return;
        }
      }
    }
  }
}

void links_hash_int_set(links_hash_t* hash, size_t key, void* value){
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
        hash->free_value(item->value);
        item->value = value;
        if(hash->max_age){
          item->expires = uv_now(uv_default_loop()) + hash->max_age;
        }else{
          item->expires = 0;
        }
        return;
      }
    }
  }

  item = (links_hash_item_t*)links_malloc(sizeof(links_hash_item_t));
  item->value = value;
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
        return item->value;
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
        hash->free_value(item->value);
        links_free(item);
        hash->items--;
        return;
      }
    }
  }
}
