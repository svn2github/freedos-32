#ifndef __FD32_DEV_H
#define __FD32_DEV_H

#include <ll/i386/hw-data.h>

#define DRIVER_TYPE_PRESENT 1
#define DRIVER_TYPE_TTY     2
#define DRIVER_TYPE_WRITE   4


#define FD32_SET_NONBLOCKING_IO 1

typedef struct Fd32DevOps
{
  struct Fd32DevOps *Next;
  DWORD              Type;
  void              *P;
}
fd32_dev_ops_t;


typedef struct Fd32Dev
{
  struct Fd32Dev *Next;
  char           *Name;
  DWORD           Type;
  DWORD           Status;
  fd32_dev_ops_t *Ops;
}
fd32_dev_t;


extern fd32_dev_t *Fd32Devices;


int   fd32_dev_register  (char *Name, DWORD Type);
int   fd32_dev_unregister(char *Name);
int   fd32_dev_add_ops   (char *Name, fd32_dev_ops_t *Ops);
int   fd32_dev_remove_ops(char *Name, DWORD Type);
void *fd32_dev_query     (char *Name, DWORD Type);
void  fd32_dev_enum(int (*dev_found_callback)(fd32_dev_t *, void *),
                    int (*ops_found_callback)(fd32_dev_ops_t *, void *),
                    void *DevParams, void *OpsParams);
int   fd32_list_devices  ();
int fd32_driver_query(char *name);

#endif /* #ifndef __FD32_DEV_H */

