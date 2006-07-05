/* Message Queues for FD32: driver initialization
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/error.h>
#include <kernel.h>

#include "msg.h"

void msgq_init(void)
{
  int res;

  message("FD/32 Message Queues: Adding syscalls\n");
  res = fd32_add_call("fd32_msg_newqueue", fd32_msg_newqueue, ADD);
  if (res < 0) {
    error("Cannot add fd32_msg_newqueue...\n");
  }
  res = fd32_add_call("fd32_msg_killqueue", fd32_msg_killqueue, ADD);
  if (res < 0) {
    error("Cannot add fd32_msg_killqueue...\n");
  }
  res = fd32_add_call("fd32_msg_enqueue", fd32_msg_enqueue, ADD);
  if (res < 0) {
    error("Cannot add fd32_msg_enqueue...\n");
  }
  res = fd32_add_call("fd32_msg_dequeue", fd32_msg_dequeue, ADD);
  if (res < 0) {
    error("Cannot add fd32_msg_dequeue...\n");
  }
}

