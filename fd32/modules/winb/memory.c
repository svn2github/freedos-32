#include <stdlib.h>
#include <windows.h>
#include "kmem.h"

/* need current_psp? and memlimit? */

void * malloc( size_t n_bytes )
{
	DWORD * mem_size;
	void * res;
	
	res = (void *)mem_get(n_bytes + sizeof(DWORD));
	if (res == NULL) {
		return NULL;
	}
	/* Save the memory size... */
	mem_size = (DWORD *)res;
	mem_size[0] = n_bytes;
	
	return res + sizeof(DWORD);
}

void free( void * mem )
{
	DWORD * mem_size;

	if (mem == 0)
		return;

	mem_size = (DWORD *)(mem-sizeof(DWORD));
	mem_free((DWORD)mem-sizeof(DWORD), mem_size[0]);

	return;
}
