/* Mini USER32
 *
 * This is free software; see GPL.txt
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

#include "winb.h"

/* The MessageBeep function plays a waveform sound. The waveform sound for each sound type is identified by an entry in the [sounds] section of the registry. 
 *
 * Return Values
 * If the function succeeds, the return value is nonzero.
 * If the function fails, the return value is zero. To get extended error information, call GetLastError.
 */
static BOOL WINAPI fd32_imp__MessageBeep(UINT uType)
{
  return FALSE;
}

static char user32_name[] = "user32.dll";
static struct symbol user32_symarray[] = {
  {"MessageBeep",                 (uint32_t)fd32_imp__MessageBeep}
};
static uint32_t user32_symnum = sizeof(user32_symarray)/sizeof(struct symbol);;

void install_user32(void)
{
  add_dll_table(user32_name, HANDLE_OF_USER32, user32_symnum, user32_symarray);
}
