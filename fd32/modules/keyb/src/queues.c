/* Keyborad Driver for FD32
 * by Luca Abeni and Hanzac Chen
 *
 * 2004 - 2005
 * This is free software; see GPL.txt
 */

#include <dr-env.h>

#include "key.h"
#include "queues.h"

/* Level 1 keybuf queue */
static volatile BYTE rawqueue[RAWQUEUE_MAX_SIZE];
/* static volatile int rawqueue_elements; */
static volatile WORD rawqueue_head;
static volatile WORD rawqueue_tail;

void rawqueue_put(BYTE code)
{
	if (rawqueue_tail-rawqueue_head == RAWQUEUE_MAX_SIZE) {
		return;
	} /* Note: keyqueue_elements++ is not used, elements determined by keyqueue_tail[0]-keyqueue_head[0] */

	fd32_cli();
	rawqueue_tail = (rawqueue_tail + 1) % RAWQUEUE_MAX_SIZE;
	rawqueue[rawqueue_tail] = code;
	fd32_sti();

	return;
}

BYTE rawqueue_get(void)
{
	if (rawqueue_tail == rawqueue_head) {
		//    fd32_error("??? No char in queue???\n");
		return 0;
	}
	/* rawqueue_elements--; */
	fd32_cli();
	rawqueue_head = (rawqueue_head + 1) % RAWQUEUE_MAX_SIZE;
	fd32_sti();

	return rawqueue[rawqueue_head];
}

/* Note: KEYBOARD BUFFER HEAD and TAIL pointer to the buffer offset relative to 0x0040:0000
 *       So we need to subtract 0x1E (which is BDA_OFFSET(BDA_KEYB_BUF)) to get the index
 */
void keyqueue_put(WORD code)
{
	if (bios_da.keyb_buftail-bios_da.keyb_bufhead == KEYQUEUE_MAX_SIZE) {
		return;
	} else {
		DWORD i = bios_da.keyb_buftail-BDA_OFFSET(bios_da.keyb_buf);
		/* Note: keyqueue_elements++ is not used, elements determined by keyqueue_tail[0]-keyqueue_head[0] */
		fd32_cli();
		/* Put new WORD */
		bios_da.keyb_buf[i] = code&0x00FF;
		bios_da.keyb_buf[i+1] = code>>8;
		bios_da.keyb_buftail = (i + 2) % KEYQUEUE_MAX_SIZE;
		bios_da.keyb_buftail += BDA_OFFSET(bios_da.keyb_buf);
		fd32_sti();
	
		return;
	}
}

/* Note: Not ignore the maskable interrupts 'cause no shared data manipulation */
WORD keyqueue_gethead(void)
{
	if (bios_da.keyb_buftail == bios_da.keyb_bufhead) {
		/* fd32_error("??? No char in queue???\n"); */
		return 0;
	} else {
		DWORD i = (bios_da.keyb_bufhead-BDA_OFFSET(bios_da.keyb_buf)) % KEYQUEUE_MAX_SIZE;
		return bios_da.keyb_buf[i]|bios_da.keyb_buf[i+1]<<8;
	}
}

BYTE keyqueue_get(void)
{
	if (bios_da.keyb_buftail == bios_da.keyb_bufhead) {
		/* fd32_error("??? No char in queue???\n"); */
		return 0;
	} else {
		DWORD i = bios_da.keyb_bufhead-BDA_OFFSET(bios_da.keyb_buf);
		/* keyqueue_elements--; */
		fd32_cli();
		bios_da.keyb_bufhead = (bios_da.keyb_bufhead-BDA_OFFSET(bios_da.keyb_buf) + 1) % KEYQUEUE_MAX_SIZE;
		bios_da.keyb_bufhead += BDA_OFFSET(bios_da.keyb_buf);
		fd32_sti();

		return bios_da.keyb_buf[i];
	}
}

int keyqueue_empty(void)
{
	if (bios_da.keyb_buftail == bios_da.keyb_bufhead) {
		/* fd32_error("??? No char in queue???\n"); */
		return 1;
	} else {
		return 0;
	}
}

void keyb_queue_clear(void)
{
	/* Rawqueue clear */
	rawqueue_head = 0;
	rawqueue_tail = 0;
	/* Keyqueue clear */
	bios_da.keyb_bufhead = BDA_OFFSET(bios_da.keyb_buf);
	bios_da.keyb_buftail = BDA_OFFSET(bios_da.keyb_buf);
	/* Set the Keyqueue buffer start offsets to 0x0040:0000 */
	bios_da.keyb_bufstart = BDA_OFFSET(bios_da.keyb_buf);
	bios_da.keyb_bufend = BDA_OFFSET(bios_da.keyb_buf)+KEYQUEUE_MAX_SIZE;
}
