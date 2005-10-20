#ifndef __QUEUES_H__
#define __QUEUES_H__

#define QUEUE_MAX_SIZE 32U	/* BIOS Data Area limit to 32 bytes */
#define BDA_OFFSET(addr) (((DWORD)addr)&0x00FF)

void rawqueue_put(BYTE code);
BYTE rawqueue_get(void);
BYTE rawqueue_read(void);
BYTE rawqueue_check(void);

void keyqueue_put(WORD code);
BYTE keyqueue_check(void);
BYTE keyqueue_get(void);
int keyqueue_empty(void);
void keyb_queue_clear(void);

#endif
