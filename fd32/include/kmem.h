/* Sample boot code
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */
#ifndef __KMEM_H__
#define __KMEM_H__

#define pmm_add_region pmm_free
#define pmm_remove_region pmm_alloc_address

void mem_init(void *mbp);
void dosmem_init(DWORD base, DWORD size);
int dosmem_get_region(DWORD base, DWORD size);
DWORD dosmem_get(DWORD amount);
int dosmem_free(DWORD base, DWORD size);
int mem_get_region(DWORD base, DWORD size);
DWORD mem_get(DWORD amount);
int mem_free(DWORD base, DWORD size);
void mem_dump(void);
#endif
