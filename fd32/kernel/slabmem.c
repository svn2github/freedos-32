/* The FreeDOS-32 Slab Memory Allocator
 * Copyright (C) 2005  Salvatore ISAJA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file GPL.txt; if not, write to
 * the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/** \file
 *  \brief Implementation of a simple portable Slab Memory Allocator for FreeDOS-32.
 */
#include "slabmem.h"

#if 0 /* Standard libc */
 #include <stdlib.h> /* NULL, malloc, free */
 #define SLABMEM_PAGE_SIZE       4096
 #define SLABMEM_PAGE_ALLOC()    malloc(SLABMEM_PAGE_SIZE)
 #define SLABMEM_PAGE_FREE(page) free(page)
#else /* Built in FreeDOS-32 */
 #include <kmem.h> /* mem_get, mem_free */
 #include <ll/i386/stdlib.h> /* NULL */
 #define SLABMEM_PAGE_SIZE       4096
 #define SLABMEM_PAGE_ALLOC()    (void *) mem_get(SLABMEM_PAGE_SIZE)
 #define SLABMEM_PAGE_FREE(page) mem_free((DWORD) page, SLABMEM_PAGE_SIZE)
#endif


/**
 * \param cache the slab cache where to allocate the object from.
 * \return A pointer to the object, or NULL on failure.
 * \remarks The object should be released with #slabmem_free when no longer used.
 */
void *slabmem_alloc(slabmem_t *cache)
{
	void *res = cache->free_objs;
	if (!res)
	{
		size_t count = (SLABMEM_PAGE_SIZE - sizeof(void *)) / cache->obj_size - 1;
		void *obj;
		void *page = SLABMEM_PAGE_ALLOC();
		if (!page) return NULL;
		for (obj = page; count; count--, obj += cache->obj_size)
			*((void **) obj) = obj + cache->obj_size;
		*((void **) obj) = NULL;
		cache->free_objs = page;
		*((void **) (page + SLABMEM_PAGE_SIZE - sizeof(void *))) = cache->pages;
		cache->pages = page;
		res = page;
	}
	cache->free_objs = *((void **) res);
	return res;
}


/**
 * \param cache the slab cache where the object was allocated from;
 * \param obj   pointer to the object to release.
 */
void slabmem_free(slabmem_t *cache, void *obj)
{
	*((void **) obj) = cache->free_objs;
	cache->free_objs = obj;
}


/**
 * \param cache the slab cache to initialize;
 * \param obj_size the size in bytes of each object in the cache.
 * \remarks This function just sets the fields of the specified #slabmem_t
 * to reasonable default values. You can do that manually if you prefer.
 */
void slabmem_create(slabmem_t *cache, size_t obj_size)
{
	cache->pages     = NULL;
	cache->obj_size  = obj_size;
	cache->free_objs = NULL;
}


/**
 * \param cache the slab cache to destroy.
 * \remarks This functions deallocates every object in the slab cache,
 * thus, it should be called when there are no active references to any object.
 */
void slabmem_destroy(slabmem_t *cache)
{
	if (cache->pages)
	{
		void *page, *next;
		for (page = cache->pages; page; page = next)
		{
			next = *((void **) (page + SLABMEM_PAGE_SIZE - sizeof(void *)));
			SLABMEM_PAGE_FREE(page);
		}
	}
	cache->pages = NULL;
	cache->free_objs = NULL;
}
