#ifndef __MSG_H__
#define __MSG_H__

#define MSG_NULL_TYPE   0x0
#define MSG_KEYBD_TYPE  0x10
#define MSG_MOUSE_TYPE  0x20

struct fd32_msgqueue;

struct fd32_msgqueue *fd32_msg_newqueue(int size);
void fd32_msg_killqueue(struct fd32_msgqueue *q);
DWORD fd32_msg_enqueue(struct fd32_msgqueue *q, DWORD type, DWORD data);
DWORD fd32_msg_dequeue(struct fd32_msgqueue *q, DWORD *type, DWORD *data);

#endif	/* __MSG_H__ */
