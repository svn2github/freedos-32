#include <ll/i386/hw-data.h>
#include <ll/i386/string.h>
#include <ll/i386/cons.h>
#include <ll/ctype.h>

#include "kmem.h"
#include "devices.h"
#include "dev/fs.h"
#include "dev/char.h"
#include "drives.h"


struct cds
{
  struct cds *next;
  char drive;
  char device_name[FD32_MAX_LFN_LENGTH];
};

static struct cds *cds_list = NULL;
static char        DefaultDrive = 'C';

/* Returns the file name without the drive specification part */
char *get_name_without_drive(char *file_name)
{
  char *c;
  
  c = file_name;
  while ((*c != ':') && (*c != 0)) {
    c++;
  }

  if (*c == ':') {
    return c + 1;
  }

  return file_name;
}

void *get_drive(char *file_name)
{
  int i;
  char  drive_letter = 0;
  char device_name[FD32_MAX_LFN_LENGTH];
  struct cds *p;

  if (file_name[1] == ':') {
    drive_letter = toupper(file_name[0]);
  } else {
    for (i = 0; i < strlen(file_name); i++) {
      if (file_name[i] == ':') {
        memcpy(device_name, file_name, i);
	device_name[i] = 0;
      }
    }
    if (device_name[0] == 0) {
      drive_letter = DefaultDrive;
    }
  }

  if (drive_letter != 0) {
    for (p = cds_list; p != NULL; p = p->next) {
      if (drive_letter == p->drive) {
	strcpy(device_name, p->device_name);
      }
    }
  }

  return fd32_dev_query(device_name, FD32_DEV_FSVOL);
}


/* Adds a drive in the Current Directory Structure list.      */
/* Returns 0 on success, or a negative error code on failure. */
int fd32_add_drive(char drive_name, char *device_name)
{
  struct cds *p;

  p = (struct cds *)mem_get(sizeof(struct cds));
  if (p == NULL) {
    return -1;
  }
  p->drive = toupper(drive_name);
  strcpy(p->device_name, device_name);
  p->next = cds_list;
  cds_list = p;
  return 0;
}



/*
 * Default Drive handling...
 * This should be changed, in order to use the BIOS numbering
 * and to initialize the default drive according to the 
 * MultiBoot Info
 */

/* The SET DEFAULT DRIVE system call.                 */
/* Returns the number of available drives on success, */
/* or a negative error code on failure.               */
int fd32_set_default_drive(char Drive)
{
  /* FIX ME: LASTDRIVE should be read frm Config.sys */
  if ((toupper(Drive) < 'A') || (toupper(Drive) > 'Z'))
    return FD32_ERROR_UNKNOWN_UNIT;
  DefaultDrive = toupper(Drive);
  /* FIX ME: From the RBIL (INT 21h, AH=0Eh)                          */
  /*         under DOS 3.0+, the return value is the greatest of 5,   */
  /*         the value of LASTDRIVE= in CONFIG.SYS, and the number of */
  /*         drives actually present.                                 */
  return 'Z' - 'A' + 1;
}


/* The GET DEFAULT DRIVE system call.               */
/* Returns the letter of the current default drive. */
char fd32_get_default_drive()
{
  return DefaultDrive;
}
