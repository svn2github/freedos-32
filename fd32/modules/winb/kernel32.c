/* Mini KERNEL32
 * by Hanzac Chen
 *
 * This is free software; see GPL.txt
 */


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "winb.h"


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
  fd32_log_printf("[WINB] SetUnhandledExceptionFilter: %x\n", filter);
  LPTOP_LEVEL_EXCEPTION_FILTER old = top_filter;
  top_filter = filter;
  return old;
}

static LPVOID STDCALL fd32_imp__VirtualAlloc( LPVOID lpAddress, size_t dwSize, DWORD flAllocationType, DWORD flProtect )
{
  if (mem_get_region((uint32_t)lpAddress, dwSize+sizeof(DWORD)) == 1) {
    DWORD *pd = (DWORD *) lpAddress;
    pd[0] = dwSize+sizeof(DWORD);
    return pd+1;
  } else {
    return NULL;
  }
}

static BOOL STDCALL fd32_imp__VirtualFree( LPVOID lpAddress, size_t dwSize, DWORD dwFreeType )
{
  DWORD *pd = (DWORD *)lpAddress;
  if (pd != NULL) {
    mem_free((uint32_t)(pd-1), pd[0]);
    return TRUE;
  } else {
    return FALSE;
  }
}

static LPVOID STDCALL fd32_imp__LocalAlloc( UINT uFlags, size_t uBytes )
{
  DWORD *pd = (DWORD *)mem_get(uBytes);
  if (pd != NULL) {
    pd[0] = uBytes+sizeof(DWORD);
    return pd+1;
  } else {
    return NULL;
  }
}

/*
 * The LocalFree function frees the specified local memory object and invalidates its handle. 
 *
 * Return Values
 * If the function succeeds, the return value is NULL.
 * If the function fails, the return value is equal to the handle of the local memory object. 
 */
static HLOCAL STDCALL fd32_imp__LocalFree( HLOCAL hMem )
{
  DWORD *pd = (DWORD *)hMem;
  if (pd != NULL) {
    mem_free((uint32_t)(pd-1), pd[0]);
    return NULL;
  } else {
    return hMem;
  }
}


static char kernel32_name[] = "kernel32.dll";
static uint32_t kernel32_handle = 0x01;
static uint32_t kernel32_symnum = 0x10;
static struct symbol kernel32_symarray[0x10] = {
  {"AddAtomA",         (uint32_t)fd32_imp__AddAtomA},
  {"ExitProcess",      (uint32_t)fd32_imp__ExitProcess},
  {"FindAtomA",        (uint32_t)fd32_imp__FindAtomA},
  {"GetAtomNameA",     (uint32_t)fd32_imp__GetAtomNameA},
  {"GetModuleHandleA", (uint32_t)fd32_imp__GetModuleHandleA},
  {"SetUnhandledExceptionFilter", (uint32_t)fd32_imp__SetUnhandledExceptionFilter},
  {"LocalAlloc",       (uint32_t)fd32_imp__LocalAlloc},
  {"LocalFree",        (uint32_t)fd32_imp__LocalFree},
  {"VirtualAlloc",     (uint32_t)fd32_imp__VirtualAlloc},
  {"VirtualFree",      (uint32_t)fd32_imp__VirtualFree}
};

void add_kernel32(void)
{
  add_dll_table(kernel32_name, kernel32_handle, kernel32_symnum, kernel32_symarray);
}

