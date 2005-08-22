/* Sample boot code
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __MODS_H__
#define __MODS_H__

struct mods_struct {
  void *mod_start;
  void *mod_end;
  char *string;
  DWORD reserved;
};

void process_modules(int mods_count, DWORD mods_addr);
void module_address(DWORD mods_addr, int i, DWORD *start, DWORD *end);
char *module_cl(DWORD mods_addr, int i);
void mem_release_module(DWORD mstart, int m, int mnum);

#endif
