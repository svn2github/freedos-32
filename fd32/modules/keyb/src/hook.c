/* Keyborad Driver for FD32, Keyb Hook
 * by Hanzac Chen
 *
 * 2004 - 2005
 * This is free software; see GPL.txt
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
