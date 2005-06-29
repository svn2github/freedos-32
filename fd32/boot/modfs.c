/* FD/32 Module PseudoFS
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/mem.h>
#include <ll/i386/error.h>

#include "format.h"
#include "mods.h"

/* TODO: need a modfs_close? */
static DWORD start;
static DWORD end;
static DWORD pointer;
static int n = 0;

static int modfs_read(int file, void *buff, int size)
{
  int len;

  len = size;
  if (pointer + len > end) {
    len = end - pointer;
  }

  memcpy(buff, (void *)pointer, len);

  pointer += len;

  return len;
}

static int modfs_seek(int file, int pos, int wence)
{
  int res;

  if (wence == 0) {
    res = pos;
  } else if (wence == 1) {
    res = pointer + pos - start;
  } else if (wence == 2) {
    if (end - start < pos) {
      res = 0;
    } else {
      res = end - start - pos;
    }
  } else {
    res = pointer - start;
  }

  if (start + res > end) {
    res = end - start;
  }
  pointer = start + res;
  
  return res;
}

static int modfs_size(int file)
{
  int res;

  res = end - start;
  return res;
}

void modfs_open(DWORD addr, int mod_index)
{
  module_address(addr, mod_index, &start, &end);
  pointer = start;
}

void modfs_init(struct kern_funcs *kf, DWORD addr, int count)
{
  n = count;

  kf->file_seek = modfs_seek;
  kf->file_read = modfs_read;
}
