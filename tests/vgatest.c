#include <dpmi.h>
#include <stdio.h>
#include <conio.h>

unsigned char *p = (unsigned char)0xA0000;

int main(void)
{
  int i;
  __dpmi_regs regs;
  regs.h.ah = 0x00;		/* set mode */
  regs.h.al = 0x13;
  __dpmi_int(0x10, &regs);
  
  for (i = 0; i < 100; i++)
    p[i] = 0xF;
  cprintf("Hello World!\n");

  getchar();
  return 0;
}
