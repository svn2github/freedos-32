/* Mini OLEAUT32
 *
 * This is free software; see GPL.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "winb.h"

/* from wtypes.h */
typedef WCHAR OLECHAR;
typedef OLECHAR *BSTR;

/* from basetyps.h */
#define STDAPICALLTYPE      STDCALL
#define STDAPI_(t)          t STDAPICALLTYPE
/* from oleauto.h */
#define WINOLEAUTAPI_(type) STDAPI_(type)


static WINOLEAUTAPI_(BSTR) fd32_imp__SysAllocStringByteLen(LPCSTR psz, UINT len)
{
  DWORD *p;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SysAllocStringByteLen: %s (%lx) %d WCHAR size: %d\n", psz, (DWORD)psz, len, sizeof(WCHAR));
#endif
  p = malloc(len+4);
  /* Save the length of BSTR, general implementation of BSTR */
  *p++ = len;
  /* Direct copy the psz to newly-allocated address */
  if (psz != NULL)
    memcpy(p, psz, len);
  return (BSTR)p;
}


static WINOLEAUTAPI_(void) fd32_imp__SysFreeString(BSTR bstr)
{
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SysFreeString: %lx\n", (DWORD)bstr);
#endif
  free((DWORD *)bstr-1);
}


/* SysStringByteLen Returns the length (in bytes) of a BSTR. Valid for 32-bit systems only.
 *
 * Return Value
 * The number of bytes in bstr, not including a terminating null character. If the bstr parameter is NULL then zero is returned.
 */
static WINOLEAUTAPI_(UINT) fd32_imp__SysStringByteLen(BSTR bstr)
{
  UINT len = 0;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] SysStringByteLen: %lx\n", (DWORD)bstr);
#endif
  if (bstr != NULL)
    len = *((DWORD *)bstr-1);
  return len;
}


static char oleaut32_name[] = "oleaut32.dll";
static struct symbol oleaut32_symarray[] = {
  {"SysAllocStringByteLen",       (uint32_t)fd32_imp__SysAllocStringByteLen},
  {"SysFreeString",               (uint32_t)fd32_imp__SysFreeString},
  {"SysStringByteLen",            (uint32_t)fd32_imp__SysStringByteLen}
};
static uint32_t oleaut32_symnum = sizeof(oleaut32_symarray)/sizeof(struct symbol);;

void install_oleaut32(void)
{
  add_dll_table(oleaut32_name, HANDLE_OF_OLEAUT32, oleaut32_symnum, oleaut32_symarray);
}
