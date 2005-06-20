/* Header file for the FreeDOS-32 Slab Memory Allocator
 * by Salvo Isaja, 2005-06-20
 */
#ifndef __FD32_SLABMEM_H
#define __FD32_SLABMEM_H

#include <ll/sys/types.h> /* size_t */
//#include <stddef.h> /* size_t */

/** \file
 *  \brief A simple slab cache memory allocator.
 */

/// Typedef shortcut for struct slabmem_t.
typedef struct slabmem_t slabmem_t;

/// Structure identifying a slab cache.
struct slabmem_t
{
	/// Head of the list of memory pages used by the slab cache. Initialize with NULL.
	void  *pages;
	/// The size in bytes of each element in the slab cache.
	size_t obj_size;
	/// Head of the list of free objects in the slab cache. Initialize with NULL.
	void  *free_objs;
};

/// Allocates an object from a slab cache.
void *slabmem_alloc(slabmem_t *cache);

/// Releases an object to a slab cache.
void slabmem_free(slabmem_t *cache, void *obj);

/// Initializes a slab cache.
void slabmem_create(slabmem_t *cache, size_t obj_size);

/// Releases all memory used by a slab cache.
void slabmem_destroy(slabmem_t *cache);

#endif /* #ifndef __FD32_SLABMEM_H */
