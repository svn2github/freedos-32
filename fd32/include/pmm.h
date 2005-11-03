/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __PMM_H__
#define __PMM_H__

struct memheader {
  struct memheader *next;
  DWORD size;
};

struct mempool {
  struct memheader top;
};

void pmm_init(struct mempool *mp);
/*
void pmm_add_region(struct mempool *mp, DWORD base, DWORD size);
void pmm_remove_region(struct mempool *mp, DWORD base, DWORD size);
*/
int  pmm_alloc_address(struct mempool *mp, DWORD base, DWORD size);
DWORD pmm_alloc(struct mempool *mp, DWORD size);
int pmm_free(struct mempool *mp, DWORD base, DWORD size);
void pmm_dump(struct mempool *mp);

#endif /* __PMM_H__ */
