#ifndef __FD32_DPMI_H
#define __FD32_DPMI_H


/* Carry Flag set/clear macros */
#define SET_CARRY   r->d.flags |= 0x0001
#define CLEAR_CARRY r->d.flags &= 0xFFFFFFFE

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

extern inline void dpmi_return(int res, union regs *r)
{
  r->x.ax = res;
  if (res < 0) {
    SET_CARRY;
  } else {
    CLEAR_CARRY;
  }
}
#endif /* __FD32_DPMI_H */

