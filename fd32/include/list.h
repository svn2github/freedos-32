/* Double-linked list management for FreeDOS-32
 * Copyright (C) 2005  Salvatore ISAJA
 * Permission to use, copy, modify, and distribute this program is hereby
 * granted, provided this copyright and disclaimer notice is preserved.
 * THIS PROGRAM IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * This is meant to be compatible with the GNU General Public License.
 */
#ifndef __FD32_LIST_H
#define __FD32_LIST_H

/**
 * \defgroup list Double-linked list management
 *
 * These facilities, heavily inspired by the C++ STL class "list", allow simple
 * management of double-linked lists.
 *
 * Double-linked list provide an effective way to manage sets of an arbitrary
 * number of elements, using for each element a link to the previous element
 * as well as a link to the next element. The meaning of "previous" and "next"
 * here is merely conventional and its definition is left to the user of the
 * list.
 *
 * A double-linked list allows to insert and delete any element in costant
 * time and to search for an element in linear time.
 * @{
 */


/** \brief Typedef shortcut for struct ListItem. */
typedef struct ListItem ListItem;

/**
 * \brief A generic item in a double-linked list.
 *
 * This structure is a template for elements to be inserted in a double-linked
 * list. You can use any item type "inherited" from this structure, that is
 * a structure including the two link pointers as its first two fields.
 */
struct ListItem
{
	ListItem *prev; ///< Pointer to the previous element
	ListItem *next; ///< Pointer to the next element
};


/** \brief Typedef shortcut for struct List. */
typedef struct List List;

/**
 * \brief A double-linked list.
 *
 * This structure represents a generic double-linked list, using ListItem
 * elements. If statically initialized, all fields must be set to NULL or zero.
 */
struct List
{
	ListItem *begin; ///< Pointer to the first element
	ListItem *end;   ///< Pointer to the last element
	size_t    size;  ///< Number of elements in the list
};


/**
 * \brief Initializes a double-linked list.
 * \param list List object to initialize.
 * \remarks This function is a helper that initializes all field of the List
 *          structure to NULL or zero. It may be convenient for non-statically
 *          allocated List structures.
 */
static inline void list_init(List *list)
{
	list->begin = NULL;
	list->end   = NULL;
	list->size  = 0;
}


/**
 * \brief Links an item to the beginning of a list.
 * \param list list where to attach the item to;
 * \param item item to make the new first element of the list.
 * \remarks The list size is grown accordingly. You may want to explicitly cast
 *          your structure to ListItem when calling this function.
 * \sa list_push_back()
 */
static inline void list_push_front(List *list, ListItem *item)
{
	item->prev = NULL;
	item->next = list->begin;
	if (list->begin) list->begin->prev = item;
		else list->end = item;
	list->begin = item;
	list->size++;
}


/**
 * \brief Links an item to the end of a list.
 * \param list list where to attach the item to;
 * \param item item to make the new last element of the list.
 * \remarks The list size is grown accordingly. You may want to explicitly cast
 *          your structure to ListItem when calling this function.
 * \sa list_push_front()
 */
static inline void list_push_back(List *list, ListItem *item)
{
	item->prev = list->end;
	item->next = NULL;
	if (list->end) list->end->next = item;
		else list->begin = item;
	list->end = item;
	list->size++;
}


/**
 * \brief Unlinks an item from the list.
 * \param list list where to remove the item from;
 * \param item item to detach from the list.
 * \remarks The list size is decreased accordingly. The item must be present
 *          in the list, otherwise the result is unspecified. The item is
 *          removed from the list but not deallocated: it is up to the caller
 *          to perform any needed cleanup. You may want to explicitly cast your
 *          structure to ListItem when calling this function.
 */
static inline void list_erase(List *list, ListItem *item)
{
	if (item->next) item->next->prev = item->prev;
		else list->end = item->prev;
	if (item->prev) item->prev->next = item->next;
		else list->begin = item->next;
	list->size--;
}

/* @} */
#endif /* #ifndef __FD32_LIST_H */
