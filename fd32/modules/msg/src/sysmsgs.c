/* Message Queues for FD32: core functions
 * by Hanzac Chen and Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include <ll/stdlib.h>

#include <kmem.h>
#include <logger.h>

#include "msg.h"

struct fd32_msgqueue 
{
  struct fd32_msg *pool;
  int head;
  int tail;
  int num;
  int size;
};

struct fd32_msg
{
        DWORD type;
        DWORD data;
};

struct fd32_msgqueue *fd32_msg_newqueue(int size)
{
  struct fd32_msg *p;
  struct fd32_msgqueue *q;

  q = (struct fd32_msgqueue *)mem_get(sizeof(struct fd32_msgqueue));
  if (q == NULL) {
    return NULL;
  }

  p = (struct fd32_msg *)mem_get(size * sizeof(struct fd32_msg));
  if (p == NULL) {
    mem_free((DWORD)q, sizeof(struct fd32_msgqueue));
    
    return NULL;
  }
  q->pool = p;
  q->head = 0;
  q->tail = 0;
  q->num = 0;
  q->size = size;

  return q;
}

void fd32_msg_killqueue(struct fd32_msgqueue *q)
{
  mem_free((DWORD)q->pool, q->size * sizeof(struct fd32_msg));
  mem_free((DWORD)q, sizeof(struct fd32_msgqueue));
}

DWORD fd32_msg_enqueue(struct fd32_msgqueue *q, DWORD type, DWORD data)
{
  if(type == MSG_NULL_TYPE) {
    return -1;	/* FIXME: Better error code??? */
  }
  
  if(q->num == q->size) {
    /* The queue is full */
    return -1;	/* FIXME: Better error code??? */
  }
	
  /* Enqueue now */
  q->pool[q->tail].type = type;
  q->pool[q->tail].data = data;
  q->tail = (q->tail + 1) % q->size;
	
  q->num++;
  fd32_log_printf("Enqueued Message %d: %lx: %lx\n", q->num, type, data);
	
  return 1;
}


/* On success return TRUE else return FALSE */
DWORD fd32_msg_dequeue(struct fd32_msgqueue *q, DWORD *type, DWORD *data)
{
  if((type == NULL) || (data == NULL)) {
    return FALSE;
  }
  
  if(q->num == 0) {
    /* The queue is empty */
    return -1;	/* FIXME: Better error code??? */
  }
  
  /* Dequeue now */
  *type = q->pool[q->head].type;
  *data = q->pool[q->head].data;
  q->head = (q->head + 1) % q->size;

  q->num--;
  return 1;
}
