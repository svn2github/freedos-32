/* Mini OLEAUT32
 *
 * This is free software; see GPL.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "winb.h"


static char oleaut32_name[] = "oleaut32.dll";
static struct symbol oleaut32_symarray[] = {
  {"RegOpenKeyExA",               (uint32_t)0},
  {"RegCloseKey",                 (uint32_t)0}
};
static uint32_t oleaut32_symnum = sizeof(oleaut32_symarray)/sizeof(struct symbol);;

void install_oleaut32(void)
{
  add_dll_table(oleaut32_name, HANDLE_OF_OLEAUT32, oleaut32_symnum, oleaut32_symarray);
}
