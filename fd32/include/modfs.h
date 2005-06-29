/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __MODFS_H__
#define __MODFS_H__

void modfs_init(struct kern_funcs *kf, DWORD addr, int count);
void modfs_open(DWORD addr, int mod_index);
int modfs_offset(int file, int offset);

#endif /* __MODFS_H__ */
