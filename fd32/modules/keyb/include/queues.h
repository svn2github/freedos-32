#ifndef __QUEUES_H__
#define __QUEUES_H__

#define RAWQUEUE_MAX_SIZE 128
#define KEYQUEUE_MAX_SIZE 32 /* BIOS Data Area limit to 32 bytes */

void rawqueue_put(BYTE code);
BYTE rawqueue_get(void);
void keyqueue_put(WORD code);
WORD keyqueue_gethead(void);
BYTE keyqueue_get(void);
int keyqueue_empty(void);

#endif
