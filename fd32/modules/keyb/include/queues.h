#ifndef __QUEUES_H__
#define __QUEUES_H__

#define RAWQUEUE_MAX_SIZE 128
#define KEYQUEUE_MAX_SIZE 128

void rawqueue_put(BYTE code);
BYTE rawqueue_get(void);
void keyqueue_put(BYTE code);
BYTE keyqueue_get(void);
#endif
