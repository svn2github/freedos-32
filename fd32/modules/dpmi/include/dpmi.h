#ifndef __FD32_DPMI_H
#define __FD32_DPMI_H


/* Carry Flag set/clear macros */
#define SET_CARRY   r->d.flags |= 0x0001
#define CLEAR_CARRY r->d.flags &= 0xFFFFFFFE

#if 0
  #define RM_SET_CARRY   r->flags |= 0x0001
  #define RM_CLEAR_CARRY r->flags &= 0xFFFFFFFE
#else
  #define RMREGS_SET_CARRY   r->x.flags |= 0x0001
  #define RMREGS_CLEAR_CARRY r->x.flags &= 0xFFFE
#endif

/* DPMI error codes */
#define DPMI_DESCRIPTOR_UNAVAILABLE   0x8011
#define DPMI_INVALID_SELECTOR         0x8022


union regs {
  struct {
    DWORD flags;
    DWORD egs;
    DWORD efs;
    DWORD ees;
    DWORD eds;
    DWORD edi;
    DWORD esi;
    DWORD ebp;
    DWORD esp;
    DWORD ebx;
    DWORD edx;
    DWORD ecx;
    DWORD eax;
  } d;
  struct {
    WORD flags, eflags;
    WORD gs, dummy1;
    WORD fs, dummy2;
    WORD es, dummy3;
    WORD ds, dummy4;
    WORD di, di_hi;
    WORD si, si_hi;
    WORD bp, bp_hi;
    WORD sp, sp_hi;
    WORD bx, bx_hi;
    WORD dx, dx_hi;
    WORD cx, cx_hi;
    WORD ax, ax_hi;
  } x;
  struct {
    BYTE flags[4];
    BYTE egs[4];
    BYTE efs[4];
    BYTE ees[4];
    BYTE eds[4];
    BYTE edi[4];
    BYTE esi[4];
    BYTE ebp[4];
    BYTE esp[4];
    BYTE bl, bh, ebx_b2, ebx_b3;
    BYTE dl, dh, edx_b2, edx_b3;
    BYTE cl, ch, ecx_b2, ecx_b3;
    BYTE al, ah, eax_b2, eax_b3;
  } h;
};


#if 0
struct rmcall{
  DWORD edi;
  DWORD esi;
  DWORD ebp;
  DWORD reserved;
  DWORD ebx;
  DWORD edx;
  DWORD ecx;
  DWORD eax;
  WORD flags;
  WORD es;
  WORD ds;
  WORD fs;
  WORD gs;
  WORD ip;
  WORD cs;
  WORD sp;
  WORD ss;
};
#endif


/* Registers structure to be used in DPMI service */
/* "Simulate real mode interrupt".                */
typedef union rmregs
{
  struct
  {
    DWORD edi;
    DWORD esi;
    DWORD ebp;
    DWORD Res;
    DWORD ebx;
    DWORD edx;
    DWORD ecx;
    DWORD eax;
  } d;
  struct
  {
    WORD di, di_hi;
    WORD si, si_hi;
    WORD bp, bp_hi;
    WORD Res, Res_hi;
    WORD bx, bx_hi;
    WORD dx, dx_hi;
    WORD cx, cx_hi;
    WORD ax, ax_hi;
    WORD flags;
    WORD es;
    WORD ds;
    WORD fs;
    WORD gs;
    WORD ip;
    WORD cs;
    WORD sp;
    WORD ss;
  } x;
  struct
  {
    BYTE edi[4];
    BYTE esi[4];
    BYTE ebp[4];
    BYTE res[4];
    BYTE bl, bh, ebx_b2, ebx_b3;
    BYTE dl, dh, edx_b2, edx_b3;
    BYTE cl, ch, ecx_b2, ecx_b3;
    BYTE al, ah, eax_b2, eax_b3;
  } h;
}
tRMRegs;


#endif /* __FD32_DPMI_H */

