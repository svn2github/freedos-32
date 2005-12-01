/* Sample boot code
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

uint32_t mmap_entry(uint32_t addr, uint32_t *bah, uint32_t *bal, uint32_t *lh, uint32_t *ll, uint32_t *type);
void mmap_process(uint32_t addr, uint32_t len, uint32_t *lb, uint32_t *ls, uint32_t *hb, uint32_t *hs);
