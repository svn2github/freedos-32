#include <ll/i386/error.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/string.h>
#include <kmem.h>
#include <devices.h>


fd32_dev_t *Fd32Devices = NULL;


/* Allocate a new device structure and prepend it to the devices list. */
/* Return zero on success, nonzero on failure.                         */
int fd32_dev_register(char *Name, DWORD Type)
{
  /* FIX ME: What if not enough memory? */
  fd32_dev_t *New = (fd32_dev_t *) mem_get(sizeof(fd32_dev_t));
  New->Next   = Fd32Devices;
  New->Name   = (char *) mem_get(strlen(Name) + 1);
  strcpy(New->Name, Name);
  New->Type   = Type;
  New->Status = 0;
  New->Ops    = NULL;
  Fd32Devices = New;
  return 0;
}


/* Remove and free the specified device from the devices list. */
/* Return zero on success, nonzero on failure.                 */
int fd32_dev_unregister(char *Name)
{
  fd32_dev_t *D;
  fd32_dev_t *Prev = NULL;

  for (D = Fd32Devices; D != NULL; Prev = D, D = D->Next)
    /* FIX ME: Should be a Unicode UTF-8 case insensitive compare */
    if (strcmp(D->Name, 0) == 0)
    {
      if (Prev == NULL) Fd32Devices = D->Next;
                   else Prev->Next  = D->Next;
      /* FIX ME: Free Ops structures too! */
      mem_free((DWORD)D->Name, strlen(D->Name) + 1);
      mem_free((DWORD)D, sizeof(fd32_dev_t));
      return 0;
    }
  return -1; /* Not found */
}


/* Add a driver interface to the specified device, */
/* prepending it to the list of ops structures.    */
/* Return zero on success, nonzero on failure.     */
int fd32_dev_add_ops(char *Name, fd32_dev_ops_t *Ops)
{
  fd32_dev_t *D;
  for (D = Fd32Devices; D != NULL; D = D->Next)
    /* FIX ME: Should be a Unicode UTF-8 case insensitive compare */
    if (strcmp(D->Name, Name) == 0)
    {
      Ops->Next = D->Ops;
      D->Ops    = Ops;
      return 0;
    }
  return -1; /* Device not found */
}


/* Remove the first driver interface that matches with the specified */
/* type from the list of ops structures of a device.                 */
/* Return zero on success, nonzero on failure.                       */
int fd32_dev_remove_ops(char *Name, DWORD Type)
{
  fd32_dev_t     *D;
  fd32_dev_ops_t *Ops;
  fd32_dev_ops_t *Prev = NULL;

  for (D = Fd32Devices; D != NULL; D = D->Next)
    /* FIX ME: Should be a Unicode UTF-8 case insensitive compare */
    if (strcmp(D->Name, Name) == 0)
      for (Ops = D->Ops; Ops != NULL; Prev = Ops, Ops = Ops->Next)
        if (Ops->Type == Type)
        {
          if (Prev == NULL) D->Ops     = Ops->Next;
                       else Prev->Next = Ops->Next;
          /* FIX ME: Free Ops structure calling the free_ops method */
          return 0;
        }
  return -1; /* Device not found */
}


/* Get the first operations structure that matches with the specified type */
/* for the specified device.                                               */
/* Return a pointer to the ops structure on success, or NULL if not found. */
void *fd32_dev_query(char *Name, DWORD Type)
{
  fd32_dev_t     *D;
  fd32_dev_ops_t *Ops;

  for (D = Fd32Devices; D != NULL; D = D->Next)
    /* FIX ME: Should be a Unicode UTF-8 case insensitive compare */
    if (strcmp(D->Name, Name) == 0)
      for (Ops = D->Ops; Ops != NULL; Ops = Ops->Next)
        if (Ops->Type == Type) return Ops;
  return NULL; /* Not found */
}


/* Scan the device list, calling user defined callback functions.          */
/* For each device, the dev_found_callback function is called.             */
/* For each ops structure of a device, ops_found_callback is called.       */
/* Generic parameters can be passed to the two functions via opaque types. */
/* dev_found_callback takes also the pointer of the fd32_dev_t structure,  */
/* while ops_found_callback takes also the pointer to the ops structure.   */
/* When one of these functions return nonzero, enumeration finishes.       */
void fd32_dev_enum(int (*dev_found_callback)(fd32_dev_t *, void *),
                   int (*ops_found_callback)(fd32_dev_ops_t *, void *),
                   void *DevParams, void *OpsParams)
{
  fd32_dev_t     *Dev;
  fd32_dev_ops_t *Ops;

  for (Dev = Fd32Devices; Dev != NULL; Dev = Dev->Next)
  {
    for (Ops = Dev->Ops; Ops != NULL; Ops = Ops->Next)
      if (ops_found_callback != NULL)
        if (ops_found_callback(Ops, OpsParams)) break;
    if (dev_found_callback != NULL)
      if (dev_found_callback(Dev, DevParams)) break;
  }
}


/* This function simply lists devices and their operations. */
/* It is a debugging function only and should not be used.  */
/* Returns the number of devices in the devices list.       */
int fd32_list_devices()
{
  fd32_dev_t     *D;
  fd32_dev_ops_t *Ops;
  int             NumDevs = 0;

  for (D = Fd32Devices; D != NULL; D = D->Next)
  {
    message("%d: %s Type: %ld Ops:", ++NumDevs, D->Name, D->Type);
    for (Ops = D->Ops; Ops != NULL; Ops = Ops->Next)
      message(" %ld", Ops->Type);
    message("\n");
  }
  return NumDevs;
}


int fd32_driver_query(char *name)
{
  fd32_dev_t *p;

  for (p = Fd32Devices; p != NULL; p = p->Next) {
    if (strcmp(p->Name, name) == 0) {
      return p->Type;
    }
  }

  return 0;
}
