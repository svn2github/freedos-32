/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __JFT_H__
#define __JFT_H__

#include "dev/char.h"

/* FD32 format of the Job File Table */
typedef struct jft
{
  fd32_dev_ops_t *Ops; /* Pointer to the driver interface to use        */
  int             Fd;  /* Integer file descriptor private to the driver */
}
jft_t;



#endif /* __JFT_H__ */
