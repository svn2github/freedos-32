#include <dr-env.h>

#include "queues.h"

static volatile BYTE rawqueue[RAWQUEUE_MAX_SIZE];
static volatile int rawqueue_elements;
static volatile int rawqueue_head;
static volatile int rawqueue_tail;

static volatile BYTE keyqueue[KEYQUEUE_MAX_SIZE];
static volatile int keyqueue_elements;
static volatile int keyqueue_head;
static volatile int keyqueue_tail;

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
  keyqueue_tail = (keyqueue_tail + 1) % RAWQUEUE_MAX_SIZE;
  keyqueue[keyqueue_tail] = code;
  
  return;
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
  keyqueue_head = (keyqueue_head + 1) % KEYQUEUE_MAX_SIZE;
  
  return keyqueue[keyqueue_head];
}
