/* Sample boot code
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

DWORD mmap_entry(DWORD addr, DWORD *bah, DWORD *bal, DWORD *lh, DWORD *ll, DWORD *type);
void mmap_process(DWORD addr, DWORD len, DWORD *lb, DWORD *ls, DWORD *hb, DWORD *hs);
