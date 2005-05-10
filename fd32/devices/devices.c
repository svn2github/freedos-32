/* Use the following defines to customize the devices engine */
//#define DEV_FD32    /* Define this to add these calls to the FD32 kernel */
//#define DEV_UNICODE /* Define this to use Unicode UTF-8 in device names  */
//#define DEV_CASE    /* Define this to use case sensitive device names    */
#define DEV_NODUP   /* Define this to forbit devices with same name      */

#include <ll/i386/hw-data.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/string.h>

#include <kmem.h>

#include <devices.h>
#ifdef DEV_UNICODE
#include <unicode.h>
#endif
#include <errors.h>
#include <kernel.h> /* strcasecmp */


typedef struct
{
  fd32_request_t *request;
  void           *DeviceId;
  const char     *Name;
  int             Prev;
  int             Next;
}
tDevice;


/* Dynamic array of FD32 devices */
#define DEVSIZESTEP 20
static unsigned DevSize  = 0;
static int      DevFirst = FD32_ENMDEV;
static int      DevLast  = FD32_ENMDEV;
static tDevice *Devices  = NULL;


/* Returns nonzero if the passed device handle is invalid */
static inline int is_invalid(unsigned Handle)
{
  if (Handle >= DevSize) return FD32_ENODEV;
  if (!(Devices[Handle].request)) return FD32_ENODEV;
  return 0;
}


/* Puts the request function, the device identifier and up to MaxName */
/* bytes of the device name of the specified device in the passed     */
/* variables. All pointers may be NULL if not needed.                 */
/* Returns 0 on success, or a negative error code on failure.         */
int fd32_dev_get(unsigned Handle, fd32_request_t **request, void **DeviceId,
                 char *Name, int MaxName)
{
  if (is_invalid(Handle)) return FD32_ENODEV;
  if (request) *request = Devices[Handle].request;
  if (DeviceId) *DeviceId = Devices[Handle].DeviceId;
  if (Name && MaxName) strncpy(Name, Devices[Handle].Name, MaxName);
  return 0;
}


/* Searches for a devices with the specified name, starting from     */
/* the last registered one. Use the DEV_CASE and DEV_UNICODE symbols */
/* to customize the behaviour of this function.                      */
/* Returns a not negative device handle on success, or FD32_ENODEV   */
/* if a device with that name is not found.                          */
int fd32_dev_search(const char *Name)
{
  int k;
  for (k = DevLast; k != FD32_ENMDEV; k = Devices[k].Prev)
  {
    #ifdef DEV_CASE
     if (strcmp(Devices[k].Name, Name) == 0) return k;
    #else
     #ifdef DEV_UNICODE
      if (utf8_stricmp(Devices[k].Name, Name) == 0) return k;
     #else
      if (strcasecmp(Devices[k].Name, Name) == 0) return k;
     #endif
    #endif
  }
  return FD32_ENODEV;
}


/* Returns the handle of the first device registered. */
int fd32_dev_first(void)
{
  return DevFirst;
}


/* Returns the handle of the last device registered. */
int fd32_dev_last(void)
{
  return DevLast;
}


/* Returns the handle of the device registered after the specified device. */
/* If the specified device is the last device, FD32_ENMDEV is returned.    */
/* On failure, returns a negative error code.                              */
int fd32_dev_next(unsigned Handle)
{
  if (is_invalid(Handle)) return FD32_ENODEV;
  return Devices[Handle].Next;
}


/* Returns the handle of the device registered before the specified device. */
/* If the specified device is the first device, FD32_ENMDEV is returned.    */
/* On failure, returns a negative error code.                               */
int fd32_dev_prev(unsigned Handle)
{
  if (is_invalid(Handle)) return FD32_ENODEV;
  return Devices[Handle].Prev;
}


/* Registers a new device in a free entry of the devices array.   */
/* If the devices array is full, it is extended to find room.     */
/* The new device is registered as last device.                   */
/* Returns a not negative device handle on success, or a negative */
/* error code on failure.                                         */
int fd32_dev_register(fd32_request_t *request, void *DeviceId, const char *Name)
{
  unsigned k;

  #ifdef DEV_NODUP
  if (fd32_dev_search(Name) >= 0) return FD32_EACCES; //FIX ME: A decent one
  #endif
  /* Search for an unused entry in the devices array */
  for (k = 0; k < DevSize; k++) if (Devices[k].request == NULL) break;
  /* If no unused entry are present, increase the devices array size */
  if (k == DevSize)
  {
    /* Reallocate the array growing it of DEVSIZESTEP entries */
    DWORD    OldSize  = DevSize * sizeof(tDevice);
    DWORD    NewSize  = (DevSize + DEVSIZESTEP) * sizeof(tDevice);
    tDevice *NewArray = (tDevice *) mem_get(NewSize);
    if (NewArray == NULL) return FD32_ENOMEM;
    memset(NewArray, 0, NewSize);
    memcpy(NewArray, Devices, OldSize);
    if (Devices) mem_free((DWORD)Devices, OldSize);
    Devices  = NewArray;
    DevSize += DEVSIZESTEP;
  }
  Devices[k].request  = request;
  Devices[k].DeviceId = DeviceId;
  Devices[k].Name     = Name;
  Devices[k].Next     = FD32_ENMDEV;
  Devices[k].Prev     = DevLast;
  if (DevLast != FD32_ENMDEV) Devices[DevLast].Next = k;
                         else DevFirst = k;
  DevLast = k;
  return (int) k;
}



/* Removes the specified device from the devices array.       */
/* Returns 0 on success, or a negative error code on failure. */
int fd32_dev_unregister(unsigned Handle)
{
  if (is_invalid(Handle)) return FD32_ENODEV;
  Devices[Handle].request = NULL;
  /* Unlink first-to-last chain */
  if (Devices[Handle].Prev != FD32_ENMDEV)
    Devices[Devices[Handle].Prev].Next = Devices[Handle].Next;
   else
    DevFirst = Devices[Handle].Next;
  /* Unlink last-to-first chain */
  if (Devices[Handle].Next != FD32_ENMDEV)
    Devices[Devices[Handle].Next].Prev = Devices[Handle].Prev;
   else
    DevLast = Devices[Handle].Prev;
  return 0;
}


/* Replaces the driver request function and the device identifier of       */
/* the specified device, without changing the device name and time order.  */
/* THIS CALL HAS TO BE USED WITH CAUTION, BACKING UP THE OLD INFORMATIONS! */
/* Returns 0 on success, or a negative error code on failure.              */
int fd32_dev_replace(unsigned Handle, fd32_request_t *request, void *DeviceId)
{
  if (is_invalid(Handle)) return FD32_ENODEV;
  Devices[Handle].request  = request;
  Devices[Handle].DeviceId = DeviceId;
  return 0;
}


#ifdef DEV_FD32
/* The following structure is used to add the Devices Engine */
/* symbols to the symbol table of the FD32 kernel.           */
static struct { char *Name; DWORD Address; } Symbols[] =
{
  { "fd32_dev_get",        (DWORD) fd32_dev_get        },
  { "fd32_dev_search",     (DWORD) fd32_dev_search     },
  { "fd32_dev_first",      (DWORD) fd32_dev_first      },
  { "fd32_dev_last",       (DWORD) fd32_dev_last       },
  { "fd32_dev_next",       (DWORD) fd32_dev_next       },
  { "fd32_dev_prev",       (DWORD) fd32_dev_prev       },
  { "fd32_dev_register",   (DWORD) fd32_dev_register   },
  { "fd32_dev_unregister", (DWORD) fd32_dev_unregister },
  { "fd32_dev_replace",    (DWORD) fd32_dev_replace    },
  { 0, 0 }
};


/* Initialize the Devices Engine adding its symbol to the kernel table */
void fd32_devices_engine_init(void)
{
  int k;

  fd32_message("Going to install the Devices Engine...\n");
  for (k = 0; Symbols[k].Name; k++)
    if (add_call(Symbols[k].Name, Symbols[k].Address, ADD) == -1)
      fd32_error("Cannot add %s to the symbol table\n", Symbols[k].Name);
  fd32_message("Done\n");
}
#endif
