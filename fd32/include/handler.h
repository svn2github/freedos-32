/* FD32 kernel
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#ifndef __FD32_HANDLER_H__
#define __FD32_HANDLER_H__
struct handler {
  WORD cs;
  DWORD eip;
};

/* Should this be here or in kernel.h? */
void farcall(WORD cs, DWORD eip);
#endif
