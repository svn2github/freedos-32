/* DPMI Driver for FD32: driver initialization
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-func.h>

extern void int0x21(void);
extern void int0x31(void);

void DPMI_init(void)
{
  IDT_place(0x21, int0x21);
  IDT_place(0x31, int0x31);
}
