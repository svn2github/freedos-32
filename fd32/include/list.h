#ifndef __FD32_LIST_H
#define __FD32_LIST_H

/**
 * \file
 * \brief Simple routines to manage double-linked lists. Inspired by the C++ STL list.
 */


/// Template structure for double-linked list items.
typedef struct ListItem ListItem;
struct ListItem
{
	ListItem *prev;
	ListItem *next;
};


/// A double-linked list of generic ListItem elements.
typedef struct List List;
struct List
{
	ListItem *begin;
	ListItem *end;
	size_t    size;
};


/// Initializes an empty double-linked list.
static inline void list_init(List *list)
{
	list->begin = NULL;
	list->end   = NULL;
	list->size  = 0;
}


/// Links an item to the beginning of a list.
static inline void list_push_front(List *list, ListItem *item)
{
	item->prev = NULL;
	item->next = list->begin;
	if (list->begin) list->begin->prev = item;
		else list->end = item;
	list->begin = item;
	list->size++;
}


/// Links an item to the end of a list.
static inline void list_push_back(List *list, ListItem *item)
{
	item->prev = list->end;
	item->next = NULL;
	if (list->end) list->end->next = item;
		else list->begin = item;
	list->end = item;
	list->size++;
}


/// Unlinks an item from the list.
static inline void list_erase(List *list, ListItem *item)
{
	if (item->next) item->next->prev = item->prev;
		else list->end = item->prev;
	if (item->prev) item->prev->next = item->next;
		else list->begin = item->next;
	list->size--;
}


#endif /* #ifndef __FD32_LIST_H */
