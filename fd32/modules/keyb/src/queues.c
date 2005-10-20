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
static BYTE rawqueue[QUEUE_MAX_SIZE];
static WORD queue_head;
static WORD queue_tail;
static WORD queue_index;

void rawqueue_put(BYTE code)
{
	fd32_cli();
	rawqueue[queue_tail] = code;
	queue_tail = (queue_tail + 1) % QUEUE_MAX_SIZE;
	/* NOTE: loop-queue, auto-loop and overwrite the oldest codes  */
	if (queue_tail == queue_head) {
		queue_head = (queue_head + 1) % QUEUE_MAX_SIZE;
	}
	fd32_sti();

	return;
}

BYTE rawqueue_get(void)
{
	if ((queue_tail-queue_head) % QUEUE_MAX_SIZE == 1) {
		/* NOTE: No char in queue??? */
		return 0;
	}
	fd32_cli();
	queue_head = (queue_head + 1) % QUEUE_MAX_SIZE;
	fd32_sti();
	return rawqueue[queue_head];
}

/* Read the rawqueue incrementally, based on queue_index from head to tail */
BYTE rawqueue_read(void)
{
	if ((queue_tail-queue_index) % QUEUE_MAX_SIZE == 1) {
		/* NOTE: No char in queue??? */
		return 0;
	}
	fd32_cli();
	queue_index = (queue_index + 1) % QUEUE_MAX_SIZE;
	fd32_sti();
	
	return rawqueue[queue_index];
}

BYTE rawqueue_check(void)
{
	if ((queue_tail-queue_head) % QUEUE_MAX_SIZE == 1) {
		/* NOTE: No char in queue??? */
		return 0;
	} else {
		DWORD i = (queue_head + 1) % QUEUE_MAX_SIZE;
		return rawqueue[i];
	}
}

/* keyqueue_put(decoded_keycode) only after rawqueue_check() succeed
 * NOTE: KEYBOARD BUFFER HEAD and TAIL pointer to the buffer offset relative to 0x0040:0000
 *	So we need to subtract 0x1E (which is BDA_OFFSET(BDA_KEYB_BUF)) to get the index
 */
void keyqueue_put(WORD code)
{
	fd32_cli();
	/* Put the keycode in the keyqueue, and update the rawqueue with the new scancode (it may differ) */
	bios_da.keyb_buf[queue_index] = (BYTE)(code);
	rawqueue[queue_index] = (BYTE)(code>>8);
	/* Update the keyqueue buffer tail */
	bios_da.keyb_buftail = (queue_index + 1) % QUEUE_MAX_SIZE;
	bios_da.keyb_buftail += BDA_OFFSET(bios_da.keyb_buf);
	fd32_sti();
}

/* Note: Not ignore the maskable interrupts 'cause no shared data manipulation */
BYTE keyqueue_check(void)
{
	if (bios_da.keyb_buftail == bios_da.keyb_bufhead) {
		/* NOTE: No char in queue??? */
		return 0;
	} else {
		DWORD i = bios_da.keyb_bufhead-BDA_OFFSET(bios_da.keyb_buf);
		return bios_da.keyb_buf[i];
	}
}

BYTE keyqueue_get(void)
{
	if (bios_da.keyb_buftail == bios_da.keyb_bufhead) {
		/* NOTE: No char in queue??? */
		return 0;
	} else {
		DWORD i = bios_da.keyb_bufhead-BDA_OFFSET(bios_da.keyb_buf);
		fd32_cli();
		bios_da.keyb_bufhead = (i + 1) % QUEUE_MAX_SIZE;
		bios_da.keyb_bufhead += BDA_OFFSET(bios_da.keyb_buf);
		fd32_sti();
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
	queue_index = queue_head = 0;
	queue_tail = 1;
	/* Keyqueue clear */
	bios_da.keyb_bufhead = BDA_OFFSET(bios_da.keyb_buf)+1;
	bios_da.keyb_buftail = BDA_OFFSET(bios_da.keyb_buf)+1;
	/* Set the Keyqueue buffer start offsets to 0x0040:0000 */
	bios_da.keyb_bufstart = BDA_OFFSET(bios_da.keyb_buf);
	bios_da.keyb_bufend = BDA_OFFSET(bios_da.keyb_buf)+QUEUE_MAX_SIZE;
}
