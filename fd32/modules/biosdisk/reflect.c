/* FD32 Interrupt reflection functionalities
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/error.h>
#include <../drivers/dpmi/include/dpmi.h>

extern DWORD rm_irq_table[256];

/* This is implemented in oslib\xlib\ctx.s */
extern CONTEXT /*__cdecl*/ context_save(void);

void fd32_int_handler(union regs r, DWORD intnum)
{
  WORD ctx;
  TSS *vm86_tss;
  DWORD *bos;
  DWORD *tos;
  DWORD isr_cs, isr_eip;
  WORD *old_esp;
  DWORD ef;
  DWORD rmint;

  tos = (DWORD *) &r;
  vm86_tss = (TSS *)vm86_get_tss();
  bos = (DWORD *)vm86_tss->esp0;

#ifdef __REFLECT_DEBUG__
  fd32_log_printf("Int %lu (0x%lx)... ", intnum, intnum);
#endif  
  ctx = context_save();

  if (ctx == X_VM86_TSS) {
#ifdef __REFLECT_DEBUG__
    fd32_log_printf("To be reflected in VM86 mode...");
#endif

    if (intnum == 0x15) {
      ef = *(bos - 7);
      ef &= 0xFFFFFFFE;
      *(bos - 7) = ef;
#ifdef __VM86_REFLECT_DEBUG__
      fd32_log_printf("\n");
#endif
      return;
    }
    
    if ((intnum >= 0x50) && (intnum < 0x58)) {
      rmint = intnum - 0x50;
    } else {
      if ((intnum < 0x70) || (intnum > 0x77)) {
	/* Error!!! Should we panic? */
	return;
      }
      rmint = intnum;
    }
    rmint += 8;

#ifdef __VM86_REFLECT_DEBUG__
    fd32_log_printf("Entering ESP: %lx (= 0x%lx)\n",
	    (DWORD)(tos + 9), vm86_tss->esp0);
    fd32_log_printf("Old EIP: 0x%lx    0x%lx\n", *(tos + 9), *(bos - 9));
    fd32_log_printf("Old CS: 0x%x\n", (WORD)(*(bos - 8)));
    fd32_log_printf("Old EFlags: 0x%lx    0x%lx\n", *(tos + 11), *(bos - 7));
    fd32_log_printf("Old ESP: 0x%lx    0x%lx\n", *(tos + 12), *(bos - 6));
    fd32_log_printf("Emulate, please!!!\n");
#endif
    old_esp = (WORD *)(*(bos - 6) + (*(bos - 5) << 4));
#ifdef __VM86_REFLECT_DEBUG__
    fd32_log_printf("Old stack (Linear): 0x%lx\n", (DWORD)old_esp);
#endif
    *(old_esp - 1) = (WORD)(*(bos - 7)) /*CPU_FLAG_VM | CPU_FLAG_IOPL*/;
    *(old_esp - 2) = (WORD)(*(bos - 8));
    *(old_esp - 3) = (WORD)(*(bos - 9));
    *(bos - 6) -= 6;
    
    isr_cs= ((rm_irq_table[rmint]) & 0xFFFF0000) >> 16;
    isr_eip = ((rm_irq_table[rmint]) & 0x0000FFFF);
#ifdef __VM86_REFLECT_DEBUG__
    fd32_log_printf("I have to call 0x%lx:0x%lx\n", isr_cs, isr_eip);
#endif
    *(bos - 8) = isr_cs;
    *(bos - 9) = isr_eip;
  }
}
