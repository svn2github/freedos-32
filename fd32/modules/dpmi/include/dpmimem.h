#ifndef __DPMIMEM_H__
#define __DPMIMEM_H__

void dpmi_set_limit(DWORD l);
DWORD dpmi_get_mem(DWORD addr, DWORD size);
DWORD dpmi_alloc(DWORD size, DWORD *base);
int dpmi_free(DWORD handle);

#endif    /* DPMIMEM_H */
