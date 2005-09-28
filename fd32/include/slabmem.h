/* Header file for the FreeDOS-32 Slab Memory Allocator
 * Copyright (C) 2005  Salvatore ISAJA
 * Permission to use, copy, modify, and distribute this program is hereby
 * granted, provided this copyright and disclaimer notice is preserved.
 * THIS PROGRAM IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * This is meant to be compatible with the GNU General Public License.
 */
#ifndef __FD32_SLABMEM_H
#define __FD32_SLABMEM_H

/**
 * \addtogroup slabmem
 * @{
 */

#include <ll/sys/types.h> /* size_t */
//#include <stddef.h> /* size_t */

/**
 * \brief Old type name for a slab cache type.
 * \deprecated Will be removed as soon as all callers have adopted the new naming convention.
 */
#define slabmem_t SlabCache

/** \brief Typedef shortcut for struct SlabCache. */
typedef struct SlabCache SlabCache;

/**
 * \brief Structure identifying a slab cache.
 * \remarks If statically initialized, the pointers must be set to NULL.
 */
struct SlabCache
{
	
	void  *pages;     ///< Head of the list of memory pages used by the slab cache.
	size_t obj_size;  ///< The size in bytes of each element in the slab cache.
	void  *free_objs; ///< Head of the list of free objects in the slab cache.
};

void *slabmem_alloc  (slabmem_t *cache);
void  slabmem_free   (slabmem_t *cache, void *obj);
void  slabmem_create (slabmem_t *cache, size_t obj_size);
void  slabmem_destroy(slabmem_t *cache);

/* @} */
#endif /* #ifndef __FD32_SLABMEM_H */
