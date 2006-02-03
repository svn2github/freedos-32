#ifndef __DPMIEXC_H__
#define __DPMIEXC_H__

#define ERROR_INVALID_VALUE        0xFFFF8021

void fd32_enable_real_mode_int(int intnum);

int fd32_get_real_mode_int(int intnum,
	WORD *segment, WORD *offset);
int fd32_set_real_mode_int(int intnum,
	WORD segment, WORD offset);
int fd32_get_exception_handler(int excnum,
	WORD *selector, DWORD *offset);
int fd32_set_exception_handler(int excnum,
	WORD selector, DWORD offset);
int fd32_get_protect_mode_int(int intnum,
	WORD *selector, DWORD *offset);
int fd32_set_protect_mode_int(int intnum,
	WORD selector, DWORD offset);

#endif /* __DPMIEXC_H__ */
