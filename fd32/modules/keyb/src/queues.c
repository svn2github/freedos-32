/* Keyborad Driver for FD32
 * original by Luca Abeni
 * extended by Hanzac Chen
 *
 * 2004 - 2005
 * This is free software; see GPL.txt
 */

#include <dr-env.h>

#include "queues.h"

static volatile BYTE rawqueue[RAWQUEUE_MAX_SIZE];
static volatile int rawqueue_elements;
static volatile int rawqueue_head;
static volatile int rawqueue_tail;

static volatile BYTE keyqueue[KEYQUEUE_MAX_SIZE];
static volatile int keyqueue_elements;
static volatile WORD *keyqueue_head = (WORD *)0x041a;
static volatile WORD *keyqueue_tail = (WORD *)0x041c;

void rawqueue_put(BYTE code)
{
  if (rawqueue_elements == RAWQUEUE_MAX_SIZE) {
    return;
  }
  rawqueue_tail = (rawqueue_tail + 1) % RAWQUEUE_MAX_SIZE;
  rawqueue_elements++;
  rawqueue[rawqueue_tail] = code;
  
  return;
}

/* Uhmm... This runs with ints enabled...
 * The only variable shared with rawqueue_put() is rawqueue_elements
 * we probably do not need this cli()/sti() stuff...
 */
BYTE rawqueue_get(void)
{
  fd32_cli();
  if (rawqueue_elements == 0) {
    //    fd32_error("??? No char in queue???\n");
    return 0;
  }
  rawqueue_elements--;
  fd32_sti();
  rawqueue_head = (rawqueue_head + 1) % RAWQUEUE_MAX_SIZE;
  
  return rawqueue[rawqueue_head];
}


void keyqueue_put(BYTE code)
{
  fd32_cli();
  if (keyqueue_elements == KEYQUEUE_MAX_SIZE) {
    fd32_sti();
    return;
  }
  keyqueue_elements++;
  fd32_sti();
  keyqueue_tail[0] = (keyqueue_tail[0] + 1) % RAWQUEUE_MAX_SIZE;
  keyqueue[keyqueue_tail[0]] = code;
  
  return;
}

BYTE keyqueue_gethead(void)
{
  fd32_cli();
  if (keyqueue_elements == 0) {
    //    fd32_error("??? No char in queue???\n");
    fd32_sti();
    return 0;
  }
  fd32_sti();
  return keyqueue[(keyqueue_head[0] + 1) % KEYQUEUE_MAX_SIZE];
}
BYTE keyqueue_get(void)
{
  fd32_cli();
  if (keyqueue_elements == 0) {
    //    fd32_error("??? No char in queue???\n");
    fd32_sti();
    return 0;
  }
  keyqueue_elements--;
  fd32_sti();
  keyqueue_head[0] = (keyqueue_head[0] + 1) % KEYQUEUE_MAX_SIZE;
  
  return keyqueue[keyqueue_head[0]];
}
