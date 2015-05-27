/**************************************************************
  @author: QianYe(coordcn@163.com)
  @overview: doubly linked list from linux kernel
  @reference: stddef.h offsetof
              kernel.h container_of
              poison.h LIST_POISON1 LIST_POISON2
 **************************************************************/

#ifndef LINKS_LIST_H
#define LINKS_LIST_H

#include "stddef.h"

#ifndef offset_of
# define offset_of(type, member) ((intptr_t)((char*)(&(((type*)(0))->member))))
#endif

#ifndef container_of
# define container_of(ptr, type, member) ((type*)((char*)(ptr) - offset_of(type, member)))
#endif

/**
  Simple doubly linked list implementation.
  Some of the internal functions ("__xxx") are useful when
  manipulating whole lists rather than single entries, as
  sometimes we already know the next/prev entries and we can
  generate better code by using them directly rather than
  using the generic single-entry routines.
 */

struct links_list_s {
  struct links_list_s* next;
  struct links_list_s* prev;
};

typedef struct links_list_s links_list_t;

static inline void links_list_init(links_list_t* list){
  list->next = list;
  list->prev = list;
}

static inline void links_list__insert(links_list_t* new, 
                                      links_list_t* prev, 
                                      links_list_t* next){
  next->prev = new;
  new->next = next;
  new->prev = prev;
  prev->next = new;
}

static inline void links_list_insert_head(links_list_t* new, links_list_t* head){
  links_list__insert(new, head, head->next);
}

static inline void links_list_insert_tail(links_list_t* new, links_list_t* head){
  links_list__insert(new, head->prev, head);
}

static inline void links_list__remove(links_list_t* prev, links_list_t* next){
  prev->next = next;
  next->prev = prev;
}

static inline void links_list_remove(links_list_t* entry){
  links_list__remove(entry->prev, entry->next);
}

static inline void links_list_remove_init(links_list_t* entry){
  links_list__remove(entry->prev, entry->next);
  links_list_init(entry);
}

static inline void links_list_move_head(links_list_t* list, links_list_t* head){
  links_list__remove(list->prev, list->next);
  links_list_insert_head(list, head);
}

static inline void links_list_move_tail(links_list_t* list, links_list_t* head){
  links_list__remove(list->prev, list->next);
  links_list_insert_tail(list, head);
}

static inline int links_list_is_empty(const links_list_t* head){
  return head->next == head;
}

static inline int links_list_is_single(const links_list_t* head){
  return !links_list_is_empty(head) && (head->next == head->prev);
}

static inline int links_list_is_last(const links_list_t* list, const links_list_t* head){
  return list->next == head;
}

/**
  links_list_entry - get the struct for this entry
  @ptr:	the &links_list_t pointer.
  @type: the type of the struct this is embedded in.
  @member:	the name of the links_list_t within the struct.
 */
#define links_list_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
  links_list_first_entry - get the first element from a list
  @ptr:	the list head to take the element from.
  @type: the type of the struct this is embedded in.
  @member:	the name of the links_list_t within the struct.
  Note that if the list is empty, it returns NULL.
 */
#define links_list_first_entry(ptr, type, member) \
	(!links_list_is_empty(ptr) ? links_list_entry((ptr)->next, type, member) : NULL)

/**
  links_list_next_entry - get the next element in list
  @pos:	the type * to cursor
  @member:	the name of the links_list_t within the struct.
 */
#define links_list_next_entry(pos, member) \
	links_list_entry((pos)->member.next, typeof(*(pos)), member)

/**
  links_list_prev_entry - get the prev element in list
  @pos:	the type * to cursor
  @member:	the name of the links_list_t within the struct.
 */
#define links_list_prev_entry(pos, member) \
	links_list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
  links_list_foreach	-	iterate over a list
  @pos:	the &links_list_t to use as a loop cursor.
  @head:	the head for your list.
 */
#define links_list_foreach(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
  links_list_foreach_reverse	-	iterate over a list backwards
  @pos:	the &links_list_t to use as a loop cursor.
  @head:	the head for your list.
 */
#define links_list_foreach_reverse(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

/**
  links_list_foreach_entry	-	iterate over list of given type
  @pos:	the type * to use as a loop cursor.
  @head:	the head for your list.
  @member:	the name of the links_list_t within the struct.
 */
#define links_list_foreach_entry(pos, head, member)				\
	for (pos = links_list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = links_list_entry(pos->member.next, typeof(*pos), member))

/**
  links_list_foreach_entry_reverse - iterate backwards over list of given type.
  @pos:	the type * to use as a loop cursor.
  @head:	the head for your list.
  @member:	the name of the list_struct within the struct.
 */
#define links_list_foreach_entry_reverse(pos, head, member)			\
	for (pos = links_list_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = links_list_entry(pos->member.prev, typeof(*pos), member))

/**
  Double linked lists with a single pointer list head.
  Mostly useful for hash tables where the two pointer list head is
  too wasteful.
  You lose the ability to access the tail in O(1).
 */

struct links_hlist_node_s {
  struct links_hlist_node_s* next;
  struct links_hlist_node_s** pprev;
};

struct links_hlist_head_s {
  struct links_hlist_node_s* first;
};

typedef struct links_hlist_node_s links_hlist_node_t;
typedef struct links_hlist_head_s links_hlist_head_t;

#define links_hlist_head_init(ptr) ((ptr)->first = NULL)

static inline void links_hlist_node_init(links_hlist_node_t* node){
  node->next = NULL;
  node->pprev = NULL;
}

static inline int links_hlist_is_empty(links_hlist_head_t* head){
  return !head->first;
}

static inline int links_hlist_is_single(links_hlist_head_t* head){
  return head->first && !head->first->next;
}

static inline void links_hlist__remove(links_hlist_node_t* node){
  if(node->pprev){
    links_hlist_node_t* next = node->next;
    links_hlist_node_t** pprev = node->pprev;
    *pprev = next;
    if(next) next->pprev = pprev;
  }
}

static inline void links_hlist_remove(links_hlist_node_t* node){
  links_hlist__remove(node);
}

static inline void links_hlist_remove_init(links_hlist_node_t* node){
  links_hlist__remove(node);
  links_hlist_node_init(node);
}

static inline void links_hlist_insert_head(links_hlist_node_t* node, links_hlist_head_t* head){
  links_hlist_node_t* first = head->first;
  node->next = first;
  if(first) first->pprev = &node->next;
  head->first = node;
  node->pprev = &head->first;
}

#define hlist_entry(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? container_of(____ptr, type, member) : NULL; \
	})

#define links_hlist_for_each(pos, head) \
	for (pos = (head)->first; pos ; pos = pos->next)

/**
  links_hlist_for_each_entry	- iterate over list of given type
  @pos:	the type * to use as a loop cursor.
  @head:	the head for your list.
  @member:	the name of the links_hlist_node_t within the struct.
 */
#define links_hlist_for_each_entry(pos, head, member)				\
	for (pos = hlist_entry((head)->first, typeof(*(pos)), member);\
	     pos;							\
	     pos = hlist_entry((pos)->member.next, typeof(*(pos)), member))

#endif /*LINKS_LIST_H*/
