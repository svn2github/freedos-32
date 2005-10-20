/* Keyboard strokes Hook for FD32's Keyborad Driver
 *
 * Copyright (C) 2005 by Hanzac Chen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <dr-env.h>

typedef enum keyb_hook_type
{
	NORMAL_HOOK = 0,
	CTRL_HOOK,
	ALT_HOOK,
	CTRL_ALT_HOOK,
} keyb_hook_type_t;

typedef void (*keyb_hook_func_t)(void);
typedef struct keyb_hook
{
	keyb_hook_func_t hook[4];
} keyb_hook_t;

static keyb_hook_t hookmap[128];

/* It is used setup the keyboard hooks
 *   WORD key is scancode
 */
void keyb_hook(WORD key, int isCTRL, int isALT, DWORD hook_func)
{
	int hook_type;

	if (isCTRL && isALT) {
		hook_type = CTRL_ALT_HOOK;
	} else if (isALT) {
		hook_type = ALT_HOOK;
	} else if (isCTRL) {
		hook_type = CTRL_HOOK;
	} else {
		hook_type = NORMAL_HOOK;
	}

	hookmap[key&0x00FF].hook[hook_type] = (keyb_hook_func_t)hook_func;
}

/* Fire the hook if exists */
void keyb_fire_hook(WORD key, int isCTRL, int isALT)
{
	int hook_type;

	if (isCTRL && isALT) {
		hook_type = CTRL_ALT_HOOK;
	} else if (isALT) {
		hook_type = ALT_HOOK;
	} else if (isCTRL) {
		hook_type = CTRL_HOOK;
	} else {
		hook_type = NORMAL_HOOK;
	}
	
	if (hookmap[key&0x00FF].hook[hook_type] != 0)
		hookmap[key&0x00FF].hook[hook_type]();
}
