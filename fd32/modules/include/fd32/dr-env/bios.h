#ifndef __FD32_DRENV_BIOS_H
#define __FD32_DRENV_BIOS_H

#include <ll/i386/hw-data.h>
#include <ll/i386/x-bios.h>

#define FD32_DECLARE_REGS(a) X_REGS16 regs_##a; \
                             X_SREGS16 selectors_##a

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
#define CS(a)    selectors_##a.cs
#define DS(a)    selectors_##a.ds
#define ES(a)    selectors_##a.es
#define GS(a)    selectors_##a.gs
#define FS(a)    selectors_##a.fs
#define SS(a)    selectors_##a.ss
#define SP(a)    regs_##a.x.sp
#define IP(a)    regs_##a.x.ip
#define FLAGS(a) regs_##a.x.cflag
#define SI(a)    regs_##a.x.si
#define DI(a)    regs_##a.x.di
#define BP(a)    regs_##a.x.bp
#define EAX(a)   regs_##a.d.eax
#define EBX(a)   regs_##a.d.ebx
#define ECX(a)   regs_##a.d.ecx
#define EDX(a)   regs_##a.d.edx

#define fd32_int(n, r) do { \
  DWORD f; \
  f = ll_fsave(); \
  vm86_callBIOS(n, &regs_##r, &regs_##r, &selectors_##r); \
  ll_frestore(f); \
} while (0)

#endif /* #ifndef __FD32_DRENV_BIOS_H */

