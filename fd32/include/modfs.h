/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __MODFS_H__
#define __MODFS_H__

void modfs_init(struct fd32_dev_char *pseudodev, DWORD addr, int count);

#endif /* __MODFS_H__ */
