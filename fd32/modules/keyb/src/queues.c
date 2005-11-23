/* Software queue for FD32's Keyborad Driver
 *
 * Copyright (C) 2003 by Luca Abeni
 * Copyright (C) 2004,2005 by Hanzac Chen
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

#include "key.h"
#include "queues.h"

/* Level 1 keybuf queue, the rawqueue has the scancode */
static WORD rawqueue_head;
static WORD rawqueue_tail;

/* NOTE:	Rawqueue share the same queue buffer with keyqueue
 *		but rawqueue use 2*i+1 bytes, keyqueue uses 2*i bytes
 */

void rawqueue_put(BYTE code)
{
	fd32_cli();
	bios_da.keyb_buf[rawqueue_tail] = code;
	rawqueue_tail = (rawqueue_tail + 2) % QUEUE_MAX_SIZE;
	/* NOTE: loop-queue, auto-loop and overwrite the oldest codes  */
	if (rawqueue_tail == rawqueue_head) {
		rawqueue_head = (rawqueue_head + 2) % QUEUE_MAX_SIZE;
	}
	fd32_sti();

	return;
}

/* Read the rawqueue incrementally, based on queue_index from head to tail */
BYTE rawqueue_read(void)
{
	WORD rawqueue_index = (bios_da.keyb_buftail-BDA_OFFSET(bios_da.keyb_buf) + 1) % QUEUE_MAX_SIZE;
	if (rawqueue_tail == rawqueue_index) {
		/* NOTE: No char in queue??? */
		return 0;
	} else {
		BYTE scancode = bios_da.keyb_buf[rawqueue_index];
		return scancode;
	}
}

static BYTE scancode;

BYTE rawqueue_check(void)
{
	return scancode;
}

BYTE rawqueue_get(void)
{
	return scancode;
}

/* keyqueue_put(decoded_keycode) only after rawqueue_read() succeed
 * NOTE: KEYBOARD BUFFER HEAD and TAIL pointer to the buffer offset relative to 0x0040:0000
 *	So we need to subtract 0x1E (which is BDA_OFFSET(BDA_KEYB_BUF)) to get the index
 */
void keyqueue_put(WORD code)
{
	WORD keyqueue_index = bios_da.keyb_buftail-BDA_OFFSET(bios_da.keyb_buf);
	WORD rawqueue_index = (keyqueue_index + 1) % QUEUE_MAX_SIZE;
	fd32_cli();
	/* Put the keycode in the keyqueue, and update the rawqueue with the new scancode (it may differ) */
	bios_da.keyb_buf[keyqueue_index] = (BYTE)(code);
	bios_da.keyb_buf[rawqueue_index] = (BYTE)(code>>8);
	/* Update the keyqueue buffer tail */
	keyqueue_index = (keyqueue_index + 2) % QUEUE_MAX_SIZE;
	bios_da.keyb_buftail = keyqueue_index;
	bios_da.keyb_buftail += BDA_OFFSET(bios_da.keyb_buf);
	fd32_sti();
}

/* Note: Not ignore the maskable interrupts 'cause no shared data manipulation */
BYTE keyqueue_check(void)
{
	if (bios_da.keyb_buftail == bios_da.keyb_bufhead) {
		/* NOTE: No char in queue??? */
		scancode = 0;
		return 0;
	} else {
		DWORD i = bios_da.keyb_bufhead-BDA_OFFSET(bios_da.keyb_buf);
		scancode = bios_da.keyb_buf[i+1];
		return bios_da.keyb_buf[i];
	}
}

BYTE keyqueue_get(void)
{
	if (bios_da.keyb_buftail == bios_da.keyb_bufhead) {
		/* NOTE: No char in queue??? */
		scancode = 0;
		return 0;
	} else {
		DWORD i = bios_da.keyb_bufhead-BDA_OFFSET(bios_da.keyb_buf);
		fd32_cli();
		bios_da.keyb_bufhead = (i + 2) % QUEUE_MAX_SIZE;
		bios_da.keyb_bufhead += BDA_OFFSET(bios_da.keyb_buf);
		fd32_sti();
		scancode = bios_da.keyb_buf[i+1];
		return bios_da.keyb_buf[i];
	}
}

int keyqueue_empty(void)
{
	if (bios_da.keyb_buftail == bios_da.keyb_bufhead) {
		/* NOTE: empty return true */
		return 1;
	} else {
		return 0;
	}
}

void keyb_queue_clear(void)
{
	/* Rawqueue clear */
	rawqueue_head = rawqueue_tail = 1;
	/* Keyqueue clear */
	bios_da.keyb_bufhead = BDA_OFFSET(bios_da.keyb_buf);
	bios_da.keyb_buftail = BDA_OFFSET(bios_da.keyb_buf);
	/* Set the Keyqueue buffer start offsets to 0x0040:0000 */
	bios_da.keyb_bufstart = BDA_OFFSET(bios_da.keyb_buf);
	bios_da.keyb_bufend = BDA_OFFSET(bios_da.keyb_buf)+QUEUE_MAX_SIZE;
}
