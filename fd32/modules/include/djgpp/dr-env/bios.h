#ifndef __FD32_DRENV_BIOS_H
#define __FD32_DRENV_BIOS_H

#include<dpmi.h>

#if 0
  #define FD32_DECLARE_REGS(a) __dpmi_regs inregs_##a, \\
                                           outregs_##a, \\
                                           selectors_##a
#else
  #define FD32_DECLARE_REGS(a) __dpmi_regs regs_##a
#endif

#define AL(a)    regs_##a.h.al
#define AH(a)    regs_##a.h.ah
#define BL(a)    regs_##a.h.bl
#define BH(a)    regs_##a.h.bh
#define CL(a)    regs_##a.h.cl
#define CH(a)    regs_##a.h.ch
#define DL(a)    regs_##a.h.dl
#define DH(a)    regs_##a.h.dh
#define AX(a)    regs_##a.x.ax
#define BX(a)    regs_##a.x.bx
#define CX(a)    regs_##a.x.cx
#define DX(a)    regs_##a.x.dx
#define CS(a)    regs_##a.x.cs
#define DS(a)    regs_##a.x.ds
#define ES(a)    regs_##a.x.es
#define GS(a)    regs_##a.x.gs
#define FS(a)    regs_##a.x.fs
#define SS(a)    regs_##a.x.ss
#define SP(a)    regs_##a.x.sp
#define IP(a)    regs_##a.x.ip
#define FLAGS(a) regs_##a.x.flags
#define SI(a)    regs_##a.x.si
#define DI(a)    regs_##a.x.di
#define BP(a)    regs_##a.x.bp
#define EAX(a)   regs_##a.d.eax
#define EBX(a)   regs_##a.d.ebx
#define ECX(a)   regs_##a.d.ecx
#define EDX(a)   regs_##a.d.edx

#define fd32_int(n, r) __dpmi_int(n, &regs_##r)

#endif /* #ifndef __FD32_DRENV_BIOS_H */

