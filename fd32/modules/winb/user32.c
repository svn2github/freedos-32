/* Mini USER32
 *
 * This is free software; see GPL.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "winb.h"


static char user32_name[] = "user32.dll";
static struct symbol user32_symarray[] = {
  {"RegOpenKeyExA",               (uint32_t)0},
  {"RegCloseKey",                 (uint32_t)0}
};
static uint32_t user32_symnum = sizeof(user32_symarray)/sizeof(struct symbol);;

void install_user32(void)
{
  add_dll_table(user32_name, HANDLE_OF_USER32, user32_symnum, user32_symarray);
}
