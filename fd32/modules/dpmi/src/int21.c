/* FIX ME: We should provide some mechanism for installing new fake */
/*         interrupt handlers (like this INT 21h handler), with the */
/*         possibility of recalling the old one.                    */


#include<ll/i386/hw-data.h>
#include <ll/i386/string.h>
#include <filesys.h>
#include <dev/fs.h>
#include <dpmi.h>
#include <stubinfo.h>
#include <logger.h>


extern struct psp *current_psp;


/* Given a FD32 return code, prepare the standard DOS return status. */
/* - on error: carry flag set, error (positive) in AX.               */
/* - on success: carry flag clear, AX destroied.                     */
static void dos_return(int Res, union rmregs *r)
{
  if (Res < 0)
  {
    RMREGS_SET_CARRY;
    r->x.ax = (WORD) (-Res);
    return;
  }
  RMREGS_CLEAR_CARRY;
  return;
}


/* Sub-handler for DOS services "Get/set file's time stamps" */
/* INT 21h, AH=57h.                                          */
static inline void get_and_set_time_stamps(union rmregs *r)
{
  int                Res;
  fd32_dev_fs_attr_t A;

  /* BX file handle */
  /* AL action      */
  switch (r->h.al)
  {
    /* Get last modification date and time */
    case 0x00:
      /* CX time in DOS format */
      /* DX date in DOS format */
      /* FIX ME: MS-DOS returns date and time of opening for char devices */
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      r->x.cx = A.MTime;
      r->x.dx = A.MDate;
      goto Success;

    /* Set last modification date and time */
    case 0x01:
      /* CX new time in DOS format */
      /* DX new date in DOS format */
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      A.MTime = r->x.cx;
      A.MDate = r->x.dx;
      Res = fd32_set_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get last access date and time */
    case 0x04:
      /* DX date in DOS format                          */
      /* CX time in DOS format (currently always 0000h) */
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      r->x.dx = A.ADate;
      r->x.cx = 0x0000;
      goto Success;

    /* Set last access date and time */
    case 0x05:
      /* CX new time in DOS format (must be 0000h) */
      /* DX new date in DOS format                 */
      if (r->x.cx != 0x0000)
      {
        Res = FD32_ERROR_INVALID_FUNCTION;
        goto Error;
      }
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      A.ADate = r->x.dx;
      Res = fd32_set_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get creation date and time */
    case 0x06:
      /* DX date in DOS format                                 */
      /* CX time in DOS format                                 */
      /* SI hundredths of seconds past the time in CX (0..199) */
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      r->x.dx = A.CDate;
      r->x.cx = A.CTime;
      r->x.si = A.CHund;
      goto Success;

    /* Set creation date and time */
    case 0x07:
      /* DX new date in DOS format                             */
      /* CX new time in DOS format                             */
      /* SI hundredths of seconds past the time in CX (0..199) */
      Res = fd32_get_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      A.CDate = r->x.dx;
      A.CTime = r->x.cx;
      A.CHund = r->x.si;
      Res = fd32_set_attributes(r->x.bx, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Invalid actions */
    default:
      Res = FD32_ERROR_INVALID_FUNCTION;
      goto Error;
  }

  Error:
    r->x.ax = (WORD) (-Res);
    RMREGS_SET_CARRY;
    return;

  Success:
    RMREGS_CLEAR_CARRY;
    return;
}


/* Sub-handler for DOS service "Get/set file's attributes" */
/* INT 21h, AH=43h.                                        */
static inline void dos_get_and_set_attributes(union rmregs *r)
{
  int                Handle, Res;
  fd32_dev_fs_attr_t A;
  /* DS:DX pointer to the ASCIZ file name */
  Handle = fd32_open((char *) (r->x.ds << 4) + r->x.dx,
                     FD32_OPEN_ACCESS_READ | FD32_OPEN_EXIST_OPEN,
                     FD32_ATTR_NONE, 0, NULL, FD32_NOLFN);
  dos_return(Handle, r);
  if (Handle < 0) return;
  /* AL contains the action */
  switch (r->h.al)
  {
    /* Get file attributes */
    case 0x00:
      /* CX attributes        */
      /* AX = CX (DR-DOS 5.0) */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      r->x.ax = r->x.cx = A.Attr;
      goto Success;

    /* Set file attributes */
    case 0x01:
      /* CX new attributes */
      Res = fd32_get_attributes(Handle, &A);
      /* FIX ME: Should close the file if currently open in sharing-compat */
      /*         mode, or generate a sharing violation critical error in   */
      /*         the file is currently open.                               */
      if (Res < 0) goto Error;
      /* Volume label and directory attributes cannot be changed.         */
      /* To change the other attributes bits of a directory the directory */
      /* flag must be clear, even if it will be set after this call.      */
      #if 0
      if (A.Attr == FD32_ATTR_VOLUMEID)
      {
        Res = FD32_ERROR_ACCESS_DENIED;
        goto Error;
      }
      if (A.Attr & FD32_ATTR_DIRECTORY)
      {
        if (r->x.cx & FD32_ATTR_DIRECTORY)
        {
          Res = FD32_ERROR_ACCESS_DENIED;
          goto Error;
        }
        A.Attr &= FD32_ATTR_DIRECTORY;
        A.Attr |= r->x.cx & ~(FD32_ATTR_VOLUMEID | FD32_ATTR_DIRECTORY);
      }
      else A.Attr = r->x.cx & ~(FD32_ATTR_VOLUMEID | FD32_ATTR_DIRECTORY);
      #else
      A.Attr = r->x.cx;
      #endif
      Res = fd32_set_attributes(Handle, &A);
      if (Res < 0) goto Error;
      goto Success;

    default:
      Res = FD32_ERROR_INVALID_FUNCTION;
      goto Error;
  }

  Error:
    r->x.ax = (WORD) (-Res);
    Res = fd32_close(Handle);
    if (Res < 0) r->x.ax = (WORD) (-Res);
    RMREGS_SET_CARRY;
    return;

  Success:
    Res = fd32_close(Handle);
    dos_return(Res, r);
    return;
}


/* Sub-handler for LFN service "Extended get/set attributes" */
/* INT 21h, AH=71h, AL=43h.                                  */
static inline void lfn_get_and_set_attributes(union rmregs *r)
{
  int                Handle, Res;
  fd32_dev_fs_attr_t A;
  /* DS:DX pointer to the ASCIZ file name */
  Handle = fd32_open((char *) (r->x.ds << 4) + r->x.dx,
                     FD32_OPEN_ACCESS_READWRITE | FD32_OPEN_EXIST_OPEN,
                     FD32_ATTR_NONE, 0, NULL, FD32_LFN);
  dos_return(Handle, r);
  if (Handle < 0) return;
  /* BL contains the action */
  switch (r->h.bl)
  {
    /* Get file attributes */
    case 0x00:
      /* CX attributes */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      r->x.cx = A.Attr;
      goto Success;

    /* Set file attributes */
    case 0x01:
      /* CX new attributes */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      A.Attr = r->x.cx;
      Res = fd32_set_attributes(Handle, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get physical size of compressed file */
    /* case 0x02: Not implemented           */

    /* Set last modification date and time */
    case 0x03:
      /* DI new date in DOS format */
      /* CX new time in DOS format */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      A.MDate = r->x.di;
      A.MTime = r->x.cx;
      Res = fd32_set_attributes(Handle, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get last modification date and time */
    case 0x04:
      /* DI date in DOS format */
      /* CX time in DOS format */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      r->x.di = A.MDate;
      r->x.cx = A.MTime;
      goto Success;

    /* Set last access date */
    case 0x05:
      /* DI new date in DOS format */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      A.ADate = r->x.di;
      Res = fd32_set_attributes(Handle, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get last access date */
    case 0x06:
      /* DI date in DOS format */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      r->x.di = A.ADate;
      goto Success;

    /* Set creation date and time */
    case 0x07:
      /* DI new date in DOS format                             */
      /* CX new time in DOS format                             */
      /* SI hundredths of seconds past the time in CX (0..199) */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      A.CDate = r->x.di;
      A.CTime = r->x.cx;
      A.CHund = r->x.si;
      Res = fd32_set_attributes(Handle, &A);
      if (Res < 0) goto Error;
      goto Success;

    /* Get creation date and time */
    case 0x08:
      /* DI date in DOS format                                 */
      /* CX time in DOS format                                 */
      /* SI hundredths of seconds past the time in CX (0..199) */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      r->x.di = A.CDate;
      r->x.cx = A.CTime;
      r->x.si = A.CHund;
      goto Success;

    /* Unsupported or invalid actions */
    default:
      r->x.ax = 0x7100; /* Function not supported */
      Res = fd32_close(Handle);
      if (Res < 0) r->x.ax = (WORD) (-Res);
      RMREGS_SET_CARRY;
      return;
  }

  Error:
    r->x.ax = (WORD) (-Res);
    Res = fd32_close(Handle);
    if (Res < 0) r->x.ax = (WORD) (-Res);
    RMREGS_SET_CARRY;
    return;

  Success:
    Res = fd32_close(Handle);
    dos_return(Res, r);
    return;
}


/* Sub-handler for Long File Names services (INT 21h AH=71h) */
static inline void lfn_functions(union rmregs *r)
{
  int Res;

  switch (r->h.al)
  {
    /* Make directory */
    case 0x39:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = fd32_mkdir((char *) (r->x.ds << 4) + r->x.dx, FD32_LFN);
      dos_return(Res, r);
      return;

    /* Remove directory */
    case 0x3A:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = fd32_rmdir((char *) (r->x.ds << 4) + r->x.dx, FD32_LFN);
      dos_return(Res, r);
      return;

    /* Delete file */
    case 0x41:
      /* DS:DX pointer to the ASCIZ file name                               */
      /* SI    use wildcard and attributes: 0000h not allowed, 0001 allowed */
      /* CL    search attributes                                            */
      /* CH    must-match attributes                                        */
      /* FIX ME: Parameter in SI is not yet used!                           */
      Res = fd32_unlink((char *) (r->x.ds << 4) + r->x.dx,
                        (DWORD) (r->h.ch << 8) + r->h.cl, FD32_LFN);
      dos_return(Res, r);
      return;

    /* Extended get/set file attributes */
    case 0x43:
      lfn_get_and_set_attributes(r);
      return;

    /* Find first matching file */
    case 0x4E:
      /* CL allowable attributes                                 */
      /* CH required attributes                                  */
      /* SI date/time format: 0000h Windows time, 0001h DOS time */
      /* DS:DX pointer to the ASCIZ file specification           */
      /* ES:DI pointer to the find data record                   */
      /* FIX ME: Parameter in SI is not yet used!                */
      Res = fd32_lfn_findfirst((char *) (r->x.ds << 4) + r->x.dx,
                               (DWORD) (r->h.ch << 8) + r->h.cl,
                               (fd32_fs_lfnfind_t *) (r->x.es << 4) + r->x.di);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) (-Res);
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* The directory handle to continue the search */
      r->x.cx = 0; /* FIX ME: Not yet implemented: Unicode conversion flag */
                   /* bit 0 set if the returned long name contains         */
                   /*  underscores for unconvertable Unicode characters    */
                   /* bit 1 set if the returned short name contains        */
                   /*  underscores for unconvertable Unicode characters    */
      return;

    /* Find next matching file */
    case 0x4F:
      /* BX    directory handle                                     */
      /* SI    date/time format: 0000h Windows time, 0001h DOS time */
      /* ES:DI pointer to the find data record                      */
      /* FIX ME: Parameter in SI should be mixed with old search flags! */
      Res = fd32_lfn_findnext(r->x.bx, 0,
                              (fd32_fs_lfnfind_t *) (r->x.es << 4) + r->x.di);
      dos_return(Res, r);
      if (Res == 0) r->x.cx = 0;
                   /* FIX ME: Not yet implemented: Unicode conversion flag */
                   /* bit 0 set if the returned long name contains         */
                   /*  underscores for unconvertable Unicode characters    */
                   /* bit 1 set if the returned short name contains        */
                   /*  underscores for unconvertable Unicode characters    */
      return;

    /* Rename of move file */
    case 0x56:
      /* DS:DX pointer to the ASCIZ name of the file to be renamed */
      /* ES:DI pointer to the ASCIZ new name for the file          */
      Res = fd32_rename((char *) (r->x.ds << 4) + r->x.dx,
                        (char *) (r->x.es << 4) + r->x.di,
                        FD32_LFN);
      dos_return(Res, r);
      return;

    /* Create or open file */
    case 0x6C:
    {
      int Action;
      /* BX    access mode and sharing flags    */
      /* CX    attribute mask for file creation */
      /* DX    action                           */
      /* DS:SI pointer to the ASCIZ file name   */
      /* DI    numeric alias hint               */
      Res = fd32_open((char *) (r->x.ds << 4) + r->x.si,
                      (DWORD) (r->x.dx << 16) + r->x.bx, r->x.cx,
                      r->x.di, &Action, FD32_LFN);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) (-Res);
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res;    /* The new handle */
      r->x.cx = (WORD) Action; /* Action taken   */
      return;
    }

    /* Get volume informations */
    case 0xA0:
      /* DS:DX pointer to the ASCIZ root directory name          */
      /* ES:DI pointer to a buffer to store the file system name */
      /* CX    size of the buffer pointed by ES:DI               */
      /* FIX ME: This function is hard coded! Should call the    */
      /*         file system driver instead!                     */
      strncpy((char *) (r->x.es << 4) + r->x.di, "FAT", r->x.cx);
      r->x.bx = (1 << 1)   /* preserve case in directory entries    */
              | (1 << 2)   /* uses Unicode characters in file names */
              | (1 << 14); /* supports DOS Long File Name fuctions  */

      r->x.cx = FD32_MAX_LFN_LENGTH;
      r->x.dx = FD32_MAX_LFN_PATH_LENGTH;
      RMREGS_CLEAR_CARRY;
      return;

    /* "FindClose" - Terminate directory search */
    case 0xA1:
      /* BX directory handle */
      Res = fd32_lfn_findclose(r->x.bx);
      dos_return(Res, r);
      return;

    /* Unsupported or invalid functions */
    default:
      RMREGS_SET_CARRY;
      r->x.ax = 0x7100;
      return;
  }
}


/* INT 21h handler for DOS file system services */
void int21_handler(union rmregs *r)
{
  int    Res;
  DWORD  dwRes;
  SQWORD sqwRes;

  switch (r->h.ah)
  {
    /* DOS 1+ - Set default drive */
    case 0x0E:
      /* DL new default drive (0='A', etc.) */
      Res = fd32_set_default_drive(r->h.dl + 'A');
      if (Res < 0) return; /* No error code is documented. Check this. */
      /* Return the number of potentially valid drive letters in AL */
      r->h.al = (BYTE) Res;
      RMREGS_CLEAR_CARRY;
      return;

    /* DOS 1+ - Get default drive */
    case 0x19:
      /* Return the default drive in AL (0='A', etc.) */
      r->h.al = fd32_get_default_drive() - 'A';
      RMREGS_CLEAR_CARRY;
      return;

    /* DOS 1+ - Set Disk Transfer Area address */
    case 0x1A:
      /* DS:DX pointer to the new DTA */
      current_psp->Dta = (void *) (r->x.ds << 4) + r->x.dx;
      return;

    /* DOS 2+ - Get Disk Transfer Area address */
    case 0x2F:
      /* ES:BX pointer to the current DTA */
      /* FIX ME: I assumed low memory DTA, I'm probably wrong! */
      r->x.es = (DWORD) current_psp->Dta >> 4;
      r->x.bx = (DWORD) current_psp->Dta & 0xF;
      return;

    /* DOS 2+ - Get DOS version */
    /* FIX ME: This call is subject to modification by SETVER */
    case 0x30:
      /* AL what to return in BH: 0=OEM number, 1=version flag */
      if (r->h.al) r->h.bh = 0x00; /* DOS is not in ROM (bit 3) */
              else r->h.bh = 0xFD; /* We claim to be FreeDOS    */
      /* Set the 24-bit User Serial Number to zero as we don't use it */
      r->h.bl = 0;
      r->x.cx = 0;
      /* We claim to be 7.10 */
      r->x.ax = 0x0A07;
      RMREGS_CLEAR_CARRY;
      return;

    /* DOS 2+ - AL = 06h - Get ture DOS version */
    case 0x33:
      if (r->h.al == 0x06)
      {
        r->x.bx = 0x0A07; /* We claim to be 7.10     */
        r->x.dx = 0;      /* Revision 0, no flags... */
        RMREGS_CLEAR_CARRY;
        return;
      }
      /* FIX ME: MS-DOS returns AL=FFh for invalid subfunctions.     */
      /*         Make this behaviour consistent with other services. */
      r->h.al = 0xFF;
      RMREGS_SET_CARRY;
      return;

    /* DOS 2+ - "MKDIR" - Create subdirectory */
    case 0x39:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = fd32_mkdir((char *) (r->x.ds << 4) + r->x.dx, FD32_NOLFN);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "RMDIR" - Remove subdirectory */
    case 0x3A:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = fd32_rmdir((char *) (r->x.ds << 4) + r->x.dx, FD32_NOLFN);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "CREAT" - Create or truncate file */
    case 0x3C:
      /* DS:DX pointer to the ASCIZ file name               */
      /* CX    attribute mask for the new file              */
      /*       (volume label and directory are not allowed) */
      Res = fd32_open((char *) (r->x.ds << 4) + r->x.dx,
                      FD32_OPEN_ACCESS_READWRITE | FD32_OPEN_SHARE_COMPAT |
                      FD32_OPEN_EXIST_REPLACE | FD32_OPEN_NOTEXIST_CREATE,
                      r->x.cx,
                      0,    /* alias hint, not used   */
                      NULL, /* action taken, not used */
                      FD32_NOLFN);
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_close(Res); /* CREAT does not leave the file open */
      dos_return(Res, r);
      return;

    /* DOS 2+ - "OPEN" - Open existing file */
    case 0x3D:
      /* DS:DX pointer to the ASCIZ file name                 */
      /* AL    opening mode                                   */
      /* CL    attribute mask for to look for (?server call?) */
      Res = fd32_open((char *) (r->x.ds << 4) + r->x.dx,
                      FD32_OPEN_EXIST_OPEN | r->h.al, FD32_ATTR_NONE,
                      0,    /* alias hint, not used   */
                      NULL, /* action taken, not used */
                      FD32_NOLFN);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) (-Res);
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* The new handle */
      return;

    /* DOS 2+ - "CLOSE" - Close file */
    case 0x3E:
      /* BX file handle */
      Res = fd32_close(r->x.bx);
      dos_return(Res, r); /* Recent DOSes preserve AH, that's OK for us */
      return;

    /* DOS 2+ - "READ" - Read from file or device */
    case 0x3F:
      /* BX    file handle                    */
      /* CX    number of bytes to read        */
      /* DS:DX pointer to the transfer buffer */
      Res = fd32_read(r->x.bx, (void *) (r->x.ds << 4) + r->x.dx, r->x.cx, &dwRes);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) (-Res);
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) dwRes; /* Return the number of bytes read in AX */
      return;

    /* DOS 2+ - "WRITE" - Write to file or device */
    case 0x40:
      /* BX    file handle                    */
      /* CX    number of bytes to write       */
      /* DS:DX pointer to the transfer buffer */
      Res = fd32_write(r->x.bx, (void *) (r->x.ds << 4) + r->x.dx, r->x.cx, &dwRes);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) (-Res);
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) dwRes; /* Return the number of bytes written in AX */
      return;

    /* DOS 2+ - "UNLINK" - Delete file */
    case 0x41:
      /* DS:DX pointer to the ASCIZ file name                  */
      /* CL    attribute mask for deletion (?server call?)     */
      /*       FIX ME: check if they are required or allowable */
      Res = fd32_unlink((char *) (r->x.ds << 4) + r->x.dx,
                        FD32_FIND_ALLOW_ALL | FD32_FIND_REQUIRE_NONE,
                        FD32_NOLFN);
      dos_return(Res, r); /* In DOS 3.3 AL seems to be the file's drive */
      return;

    /* DOS 2+ - "LSEEK" - Set current file position */
    case 0x42:
      /* BX    file handle */
      /* CX:DX offset      */
      /* AL    origin      */
      Res = fd32_lseek(r->x.bx, (SQWORD) (r->x.cx << 8) + r->x.dx,
                       r->h.al, &sqwRes);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) (-Res);
        return;
      }
      RMREGS_CLEAR_CARRY;
      /* Return the new position from start of file in DX:AX */
      r->x.ax = (WORD) sqwRes;
      r->x.dx = (WORD) (sqwRes >> 16);
      return;

    /* DOS 2+ - Get/set file attributes */
    case 0x43:
      dos_get_and_set_attributes(r);
      return;

    /* IOCTL functions - FIX ME: THIS MUST BE COMPLETELY DONE! */
    case 0x44:
/*
 * Warning!!!
 * This is clearly an hack!!!
 * The correct thing to do would be:
 * 1) Call the kernel
 * 2) The kernel converts the handle (BX) in a file using the JFT of the
 *    current task
 * 3) The kernel checks if it is a device, and so on... And returns the
 *    correct value
 */
      /* This should be GET DEVICE INFORMATION... */
      /* Hrm... We should look at al... See the FD DosDevIO (ioctl.c) */
      if ((r->x.bx == 1) || (r->x.bx == 2)) {
        /* Stdout & Stderr are TTY devices... Hope this is the
         * right way to say it... 
         */
        r->x.ax = 0x83;
        /* Ok... Got it:
         * 0x80 = File is a Device
         * 0x02 = Device is Console Output
         * 0x01 = Device is Console Input
         */
        RMREGS_CLEAR_CARRY;
        return;
      }
      r->x.ax = (WORD) (-FD32_ERROR_INVALID_HANDLE);
      RMREGS_SET_CARRY;
      return;

    /* DOS 2+ - "DUP" - Duplicate file handle */
    case 0x45:
      /* BX file handle */
      Res = fd32_dup(r->x.bx);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) (-Res);
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* Return the new handle in AX */
      return;

    /* DOS 2+ - "DUP2", "FORCEDUP" - Force duplicate file handle */
    case 0x46:
      /* BX file handle */
      /* CX new handle  */
      Res = fd32_forcedup(r->x.bx, r->x.cx);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "FINDFIRST" - Find first matching file */
    case 0x4E:
      /* AL    special flag for APPEND (ignored)                   */
      /* CX    allowable attributes (archive and readonly ignored) */
      /* DS:DX pointer to the ASCIZ file specification             */
      Res = fd32_dos_findfirst((char *) (r->x.ds << 4) + r->x.dx,
                               r->x.cx, current_psp->Dta);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "FINDNEXT" - Find next matching file */
    case 0x4F:
      Res = fd32_dos_findnext(current_psp->Dta);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "RENAME" - Rename or move file */
    case 0x56:
      /* DS:DX pointer to the ASCIZ name of the file to be renamed */
      /* ES:DI pointer to the ASCIZ new name for the file          */
      /* CL    attribute mask for server call (ignored)            */
      Res = fd32_rename((char *) (r->x.ds << 4) + r->x.dx,
                        (char *) (r->x.es << 4) + r->x.di,
                        FD32_NOLFN);
      dos_return(Res, r);
      return;

    /* DOS 2+ - Get/set file's time stamps */
    case 0x57:
      get_and_set_time_stamps(r);
      return;

    /* DOS 3.0+ - Create new file */
    case 0x5B:
      /* CX    attribute mask for the new file */
      /* DS:DX pointer to the ASCIZ file name  */
      Res = fd32_open((char *) (r->x.ds << 4) + r->x.dx,
                      FD32_OPEN_ACCESS_READWRITE |
                      FD32_OPEN_SHARE_COMPAT | FD32_OPEN_NOTEXIST_CREATE,
                      r->x.cx, 0, NULL, FD32_NOLFN);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) (-Res);
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* The new handle */
      return;

    /* DOS 3.3+ - "FFLUSH" - Commit file   */
    /* DOS 4.0+ Undocumented - Commit file */
    case 0x68:
    case 0x6A:
      /* BX file handle */
      Res = fd32_fflush(r->x.bx);
      dos_return(Res, r);
      return;

    /* DOS 5+ Undocumented - Null function */
    case 0x6B:
      r->h.al = 0;
      return;

    /* DOS 4.0+ - Extended open/create file */
    case 0x6C:
    {
      int Action;
      /* AL    00h                                                         */
      /* BL    access and sharing mode                                     */
      /* BH    flags: bit 6 = commit at every write                        */
      /*              bit 5 = return error rather than doing INT 24h       */
      /*              bit 4 = allow 4 GiB files on FAT32 volumes (ignored) */
      /* CX    attribute mask for file creation                            */
      /* DL    action: bit 4 = create file if does not exist               */
      /*               bit 1 = replace file if exists                      */
      /*               bit 0 = open file if exists                         */
      /* DH    reserved, must be 00h                                       */
      /* DS:SI pointer to the ASCIZ file name                              */
      Res = fd32_open((char *) (r->x.dx << 4) + r->x.si,
                      (DWORD) (r->x.dx << 16) + r->x.bx, r->x.cx,
                      0, &Action, FD32_NOLFN);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) (-Res);
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res;    /* The new handle */
      r->x.cx = (WORD) Action; /* Action taken   */
      return;
    }

    /* Long File Name functions */
    case 0x71:
      lfn_functions(r);
      return;

    /* Windows 95 beta - "FindClose" - Terminate directory search */
    case 0x72:
      /* BX directory handle (FIX ME: This is not documented anywhere!) */
      Res = fd32_lfn_findclose(r->x.bx);
      dos_return(Res, r);
      return;

    /* Unsupported or invalid functions */
    default:
      RMREGS_SET_CARRY;
      r->x.ax = (WORD) (-FD32_ERROR_INVALID_FUNCTION);
      return;
  }
}
