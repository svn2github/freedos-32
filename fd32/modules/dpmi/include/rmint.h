#ifndef __RMINT_H__
#define __RMINT_H__

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

#if 0
  #define RM_SET_CARRY   r->flags |= 0x0001
  #define RM_CLEAR_CARRY r->flags &= 0xFFFFFFFE
#else
  #define RMREGS_SET_CARRY   r->x.flags |= 0x0001
  #define RMREGS_CLEAR_CARRY r->x.flags &= 0xFFFE
#endif


int fd32_real_mode_int(int intnum, DWORD rmcs_address);

#endif /* __RMINT_H__ */

