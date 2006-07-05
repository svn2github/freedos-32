/* Mini ADVAPI32
 *
 * This is free software; see GPL.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "winb.h"


typedef DWORD REGSAM;
typedef HKEY *PHKEY;


/*
 * The RegOpenKeyEx function opens the specified key. 
 *
 * Return Values
 * If the function succeeds, the return value is ERROR_SUCCESS.
 * If the function fails, the return value is a nonzero error code defined in WINERROR.H. You can use the FormatMessage function with the FORMAT_MESSAGE_FROM_SYSTEM flag to get a generic description of the error.
 */
static LONG WINAPI fd32_imp__RegOpenKeyExA(HKEY hKey, LPCSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] RegOpenKeyExA: %lx %s\n", (DWORD)hKey, lpSubKey);
#endif
  return ERROR_NOT_SUPPORTED;
}


/*
 * The RegCloseKey function releases the handle of the specified key. 
 *
 * Return Values
 * If the function succeeds, the return value is ERROR_SUCCESS.
 * If the function fails, the return value is a nonzero error code defined in WINERROR.H. You can use the FormatMessage function with the FORMAT_MESSAGE_FROM_SYSTEM flag to get a generic description of the error.
 */
static LONG WINAPI fd32_imp__RegCloseKey(HKEY hKey)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] RegCloseKey: %lx\n", (DWORD)hKey);
#endif
  return ERROR_NOT_SUPPORTED;
}


static char advapi32_name[] = "advapi32.dll";
static struct symbol advapi32_symarray[] = {
  {"RegOpenKeyExA",               fd32_imp__RegOpenKeyExA},
  {"RegCloseKey",                 fd32_imp__RegCloseKey}
};
static uint32_t advapi32_symnum = sizeof(advapi32_symarray)/sizeof(struct symbol);;

void install_advapi32(void)
{
  add_dll_table(advapi32_name, HANDLE_OF_ADVAPI32, advapi32_symnum, advapi32_symarray);
}
