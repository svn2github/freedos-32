/* Mini KERNEL32
 * by Hanzac Chen
 * 
 * This is free software; see GPL.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "format.h"
#include "kernel.h"


static char *atomname = 0;
static ATOM STDCALL fd32_imp__AddAtomA( LPCTSTR str )
{
	printf("AddAtomA %s\n", str);
	atomname = malloc(strlen(str)+1);
	strcpy(atomname, str);
	return 1;
}

void restore_sp(DWORD s);
static VOID STDCALL fd32_imp__ExitProcess( UINT ecode )
{
	restore_sp(ecode);
}

static ATOM STDCALL fd32_imp__FindAtomA( LPCTSTR str )
{
	printf("FindAtomA %s\n", str);
	if(atomname == 0)
		return 0;
	
	return 1;
}

static UINT STDCALL fd32_imp__GetAtomNameA( DWORD atom, LPTSTR buffer, int nsize )
{
	strcpy(buffer, atomname);
	return nsize;
}

static DWORD STDCALL fd32_imp__GetModuleHandleA( LPCTSTR module )
{
	printf("Module Name: %s\n", module);
	return 0;
}

static PTOP_LEVEL_EXCEPTION_FILTER top_filter;
static LPVOID STDCALL fd32_imp__SetUnhandledExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER filter )
{
	LPTOP_LEVEL_EXCEPTION_FILTER old = top_filter;
	top_filter = filter;
	return old;
	return NULL;
}

static LPVOID STDCALL fd32_imp__VirtualAlloc( LPVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect )
{
	return malloc(dwSize);
}

static int STDCALL fd32_imp__VirtualFree( LPVOID lpAddress, size_t dwSize, DWORD dwFreeType )
{
	free(lpAddress);
	return 1;
}

static LPVOID STDCALL fd32_imp__LocalAlloc( UINT uFlags, size_t uBytes )
{
	return malloc(uBytes);
}

static HLOCAL STDCALL fd32_imp__LocalFree( HLOCAL hMem )
{
	free(hMem);
	return 0;
}


static char kernel32_name[] = "kernel32.dll";
static DWORD kernel32_handle = 0x01;
static DWORD kernel32_symnum = 0x10;
static struct symbol kernel32_symarray[0x10] = {
	{"AddAtomA", (DWORD)_imp__AddAtomA},
	{"ExitProcess", (DWORD)_imp__ExitProcess},
	{"FindAtomA", (DWORD)_imp__FindAtomA},
	{"GetAtomNameA", (DWORD)_imp__GetAtomNameA},
	{"GetModuleHandleA", (DWORD)_imp__GetModuleHandleA},
	{"SetUnhandledExceptionFilter", (DWORD)_imp__SetUnhandledExceptionFilter}
};

void add_kernel32(void)
{
	add_dll_table(kernel32_name, kernel32_handle, kernel32_symnum, kernel32_symarray);
}

