#ifndef __FD32_LIST_H
#define __FD32_LIST_H

/** \file
 *  Simple routines to manage double-linked lists.
 */


/// Template structure for double-linked list items.
typedef struct ListItem ListItem;
struct ListItem
{
	ListItem *prev;
	ListItem *next;
};


/// Links an item to the beginning of a list.
static inline void list_push_front(ListItem **begin, ListItem *item)
{
	item->prev = NULL;
	item->next = *begin;
	if (item->next) item->next->prev = item;
	*begin = item;
}


/// Unlinks an item from the list.
static inline void list_erase(ListItem **begin, ListItem *item)
{
	if (item->next) item->next->prev = item->prev;
	if (item->prev) item->prev->next = item->next;
		else *begin = item->next;
}


#endif /* #ifndef __FD32_LIST_H */
