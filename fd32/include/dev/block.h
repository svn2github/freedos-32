#ifndef __FD32_DEV_BLOCK_H
#define __FD32_DEV_BLOCK_H

#include <dr-env.h>


/* Ops mnemonics */
#define FD32_DEV_BLOCK 0 /* Block device */


/* Parameters structure for "get device parameters" function */
typedef struct
{
  DWORD BiosC, BiosH, BiosS;
  DWORD PhysC, PhysH, PhysS;
  DWORD BlockSize;
  DWORD TotalBlocks;
  DWORD Flags;
}
fd32_dev_block_params_t;


/* Operations structure for "Block" devices */
typedef struct
{
  fd32_dev_ops_t *Next;
  DWORD           Type;
  void           *P;
  int           (*free_ops)(fd32_dev_ops_t *Ops);

  int (*open)         (void *P);
  int (*close)        (void *P);
  int (*flush)        (void *P);
  int (*read)         (void *P, DWORD Start, DWORD Size, void *Buffer);
  int (*write)        (void *P, DWORD Start, DWORD Size, void *Buffer);
  int (*get_params)   (void *P, fd32_dev_block_params_t *Params);
  int (*ioctl)        (void *P, DWORD Function, void *Params);
  int (*add_partition)(void *P, char *Name, DWORD Start, DWORD Size);
} 
fd32_dev_block_t;


#endif /* #ifndef __FD32_DEV_BLOCK_H */

