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
  return NULL;
}


static WINOLEAUTAPI_(void) fd32_imp__SysFreeString(BSTR bstr)
{
}


static WINOLEAUTAPI_(UINT) fd32_imp__SysStringByteLen(BSTR bstr)
{
  return 0;
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
