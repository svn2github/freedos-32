/* Mini KERNEL32
 * by Hanzac Chen
 *
 * This is free software; see GPL.txt
 */


#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include "winb.h"


static LPCTSTR atomname = 0;
static ATOM STDCALL fd32_imp__AddAtomA( LPCTSTR str )
{
  printf("AddAtomA %s\n", str);
  atomname = str;
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

/* The GetCurrentProcessId function returns the process identifier of the calling process.
 *
 * Return Values
 * The return value is the process identifier of the calling process. 
 */
static DWORD WINAPI fd32_imp__GetCurrentProcessId( void )
{
  extern struct psp *current_psp;
  return (DWORD)current_psp;
}

/* The GetCurrentThreadId function returns the thread identifier of the calling thread. 
 *
 * Return Values
 * The return value is the thread identifier of the calling thread. 
 */
static DWORD WINAPI fd32_imp__GetCurrentThreadId( void )
{
  /* FD32 currently no multi-threading */
  return 0;
}

/* The GetSystemTimeAsFileTime function obtains the current system date and time. The information is in Coordinated Universal Time (UTC) format.
 *
 * Parameters
 * lpSystemTimeAsFileTime
 *   Pointer to a FILETIME structure to receive the current system date and time in UTC format. 
 */
static void WINAPI fd32_imp__GetSystemTimeAsFileTime( LPFILETIME lpftime )
{
  
}

/* The GetTickCount function retrieves the number of milliseconds that have elapsed since Windows was started. 
 *
 * Return Values
 * If the function succeeds, the return value is the number of milliseconds that have elapsed since Windows was started. 
 */
static DWORD WINAPI fd32_imp__GetTickCount( VOID )
{
  return 0;
}

/* The QueryPerformanceCounter function retrieves the current value of the high-resolution performance counter, if one exists. 
 *
 * Parameters
 * lpPerformanceCount
 *   Points to a variable that the function sets, in counts, to the current performance-counter value. If the installed hardware does not support a high-resolution performance counter, this parameter can be to zero. 
 *
 * Return Values
 * If the installed hardware supports a high-resolution performance counter, the return value is nonzero.
 * If the installed hardware does not support a high-resolution performance counter, the return value is zero. 
 */
static BOOL WINAPI fd32_imp__QueryPerformanceCounter( PLARGE_INTEGER lpcount )
{
  return FALSE;
}

static PTOP_LEVEL_EXCEPTION_FILTER top_filter;
static LPVOID STDCALL fd32_imp__SetUnhandledExceptionFilter( LPTOP_LEVEL_EXCEPTION_FILTER filter )
{
  fd32_log_printf("[WINB] SetUnhandledExceptionFilter: %lx\n", filter);
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
  {"GetCurrentProcessId", (uint32_t)fd32_imp__GetCurrentProcessId},
  {"GetCurrentThreadId",  (uint32_t)fd32_imp__GetCurrentThreadId},
  {"GetModuleHandleA", (uint32_t)fd32_imp__GetModuleHandleA},
  {"GetSystemTimeAsFileTime",     (uint32_t)fd32_imp__GetSystemTimeAsFileTime},
  {"GetTickCount",     (uint32_t)fd32_imp__GetTickCount},
  {"LocalAlloc",       (uint32_t)fd32_imp__LocalAlloc},
  {"LocalFree",        (uint32_t)fd32_imp__LocalFree},
  {"QueryPerformanceCounter",     (uint32_t)fd32_imp__QueryPerformanceCounter},
  {"SetUnhandledExceptionFilter", (uint32_t)fd32_imp__SetUnhandledExceptionFilter},
  {"VirtualAlloc",     (uint32_t)fd32_imp__VirtualAlloc},
  {"VirtualFree",      (uint32_t)fd32_imp__VirtualFree}
};

void install_kernel32(void)
{
  add_dll_table(kernel32_name, kernel32_handle, kernel32_symnum, kernel32_symarray);
}

