/* TODO: We should provide some mechanism for installing new fake   
         interrupt handlers (like this INT 21h handler), with the   
         possibility of recalling the old one.
*/


#include <ll/i386/hw-data.h>
#include <ll/i386/string.h>
#include <kernel.h>
#include <filesys.h>
#include <errors.h>
#include <stubinfo.h>
#include <logger.h>
#include "rmint.h"

/* Define the DEBUG symbol in order to activate log output */
//#define DEBUG
#ifdef DEBUG
 #define LOG_PRINTF(s) fd32_log_printf s
#else
 #define LOG_PRINTF(s)
#endif


/* The current PSP is needed for the pointer to the DTA */
extern struct psp *current_psp;

int use_lfn;


/* For direct character input INT 21h service */
/* TODO: This is only a temporary patch, replace with the correct solution */
fd32_request_t *kbdreq = NULL;


/* Parameter block for the dos_exec call, see INT 21h AH=4Bh */
typedef struct
{
  WORD  Env;
  DWORD CmdTail;
  DWORD Fcb1;
  DWORD Fcb2;
  DWORD Res1;
  DWORD Res2;
}
tExecParams;
/* DOS return code of the last executed program */
static WORD DosReturnCode;


/* Data structure for INT 21 AX=7303h                   */
/* Windows95 - FAT32 - Get extended free space on drive */
typedef struct
{
  WORD  Size;            /* Size of the returned structure (this one)  */
  WORD  Version;         /* Version of this structure (desired/actual) */
  /* The following are *with* adjustment for compression */
  DWORD SecPerClus;      /* Sectors per cluster               */
  DWORD BytesPerSec;     /* Bytes per sector                  */
  DWORD AvailClus;       /* Number of available clusters      */
  DWORD TotalClus;       /* Total number of clusters on drive */
  /* The following are *without* adjustment for compression */
  DWORD RealSecPerClus;  /* Number of sectors per cluster     */
  DWORD RealBytesPerSec; /* Bytes per sector                  */
  DWORD RealAvailClus;   /* Number of available clusters      */
  DWORD RealTotalClus;   /* Total number of clusters on drive */
  BYTE  Reserved[8];
}
__attribute__ ((packed)) tExtDiskFree;


/* Converts the passed path name to a valid path of 8.3 names,  */
/* in the OEM code page, as required for short file name calls. */
/* Returns 0 on success, or a negative error code on failure.   */
/* TODO: It is currently assumed that Source is in UTF-8.
         This is not true for DOS calls since the charset is OEM. */
static int make_sfn_path(char *Dest, const char *Source)
{
  const char *s;
  char        Comp[FD32_LFNMAX];
  char       *c;
  BYTE        FcbName[11];
  int         Res;
  #ifdef DEBUG
  char       *Save = Dest;
  #endif

  /* Copy drive specification if present */
  for (s = Source, c = Dest; *s && (*s != ':'); *(c++) = *(s++));
  if (*s)
  {
    Source = s + 1;
    *c     = ':';
    Dest   = c + 1;
  }
  /* Truncate each path component to 8.3 */
  while (*Source)
  {
    /* Extract the next partial from the path (before '\' or '/' or NULL) */
    for (c = Comp; (*Source != '\\') && (*Source != '/') && *Source; *(c++) = *(Source++));
    if (*Source) Source++;
    *c = 0;
    /* Skip consecutive slashes */
    if (c == Comp) continue;
    /* Convert the path component to a valid 8.3 name */
    if ((Res = fd32_build_fcb_name(FcbName, Comp)) < 0) return Res;
    if ((Res = fd32_expand_fcb_name(Comp, FcbName)) < 0) return Res;
    /* And append it to the Dest string, with trailing backslash if needed */
    for (c = Comp; *c; *(Dest++) = *(c++));
    if (*Source) *(Dest++) = '\\';
  }
  /* Finally add the null terminator */
  *Dest = 0;
  LOG_PRINTF(("Name shortened to '%s'\n", Save));
  return 0;
}


/* Given a FD32 return code, prepare the standard DOS return status. */
/* - on error: carry flag set, error (positive) in AX.               */
/* - on success: carry flag clear, AX destroied.                     */
static void dos_return(int Res, union rmregs *r)
{
  if (Res < 0)
  {
    RMREGS_SET_CARRY;
    /* Keeping the low 16 bits of a negative FD32 error code */
    /* yelds a positive DOS error code.                      */
    r->x.ax = (WORD) Res; 
    return;
  }
  RMREGS_CLEAR_CARRY;
  return;
}


/* Sub-handler for DOS services "Get/set file's time stamps" */
/* INT 21h, AH=57h.                                          */
static inline void get_and_set_time_stamps(union rmregs *r)
{
  int            Res;
  fd32_fs_attr_t A;

  /* BX file handle */
  /* AL action      */
  A.Size = sizeof(fd32_fs_attr_t);
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
        Res = FD32_EINVAL;
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
      Res = FD32_EINVAL;
      goto Error;
  }

  Error:
    r->x.ax = (WORD) Res;
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
  int            Handle, Res;
  fd32_fs_attr_t A;
  char           SfnPath[FD32_LFNPMAX];
  
  /* DS:DX pointer to the ASCIZ file name */
  Res = make_sfn_path(SfnPath, (char *) (r->x.ds << 4) + r->x.dx);
  dos_return(Res, r);
  if (Res < 0) return;
  Handle = fd32_open(SfnPath, FD32_OREAD | FD32_OEXIST, FD32_ANONE, 0, NULL);
  dos_return(Handle, r);
  if (Handle < 0) return;
  /* AL contains the action */
  A.Size = sizeof(fd32_fs_attr_t);
  switch (r->h.al)
  {
    /* Get file attributes */
    case 0x00:
      LOG_PRINTF(("INT 21h - Getting attributes of file \"%s\"\n", SfnPath));
      /* CX attributes        */
      /* AX = CX (DR-DOS 5.0) */
      Res = fd32_get_attributes(Handle, &A);
      if (Res < 0) goto Error;
      r->x.ax = r->x.cx = A.Attr;
      goto Success;

    /* Set file attributes */
    case 0x01:
      LOG_PRINTF(("INT 21h - Setting attributes of file \"%s\" to %04x\n", SfnPath, r->x.cx));
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
      Res = FD32_EINVAL;
      goto Error;
  }

  Error:
    r->x.ax = (WORD) Res;
    Res = fd32_close(Handle);
    if (Res < 0) r->x.ax = (WORD) Res;
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
  int            Handle, Res;
  fd32_fs_attr_t A;

  /* DS:DX pointer to the ASCIZ file name */
  Handle = fd32_open((char *) (r->x.ds << 4) + r->x.dx, FD32_ORDWR | FD32_OEXIST,
                     FD32_ANONE, 0, NULL);
  dos_return(Handle, r);
  if (Handle < 0) return;
  /* BL contains the action */
  A.Size = sizeof(fd32_fs_attr_t);
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
      if (Res < 0) r->x.ax = (WORD) Res;
      RMREGS_SET_CARRY;
      return;
  }

  Error:
    r->x.ax = (WORD) Res;
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
      Res = fd32_mkdir((char *) (r->x.ds << 4) + r->x.dx);
      dos_return(Res, r);
      return;

    /* Remove directory */
    case 0x3A:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = fd32_rmdir((char *) (r->x.ds << 4) + r->x.dx);
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
                        (DWORD) (r->h.ch << 8) + r->h.cl);
      dos_return(Res, r);
      return;

    /* Extended get/set file attributes */
    case 0x43:
      lfn_get_and_set_attributes(r);
      return;

    /* Get current directory */
    /* Same as INT 21 AH=47h, but we don't convert to 8.3 the resulting path */
    case 0x47:
    {
      /* DL    drive number (00h = default, 01h = A:, etc) */
      /* DS:SI 64-byte buffer for ASCIZ pathname           */
      char  Drive[2] = "\0\0";
      char  Cwd[FD32_LFNPMAX];
      char *Dest = (char *) (r->x.ds << 4) + r->x.si;

      Drive[0] = fd32_get_default_drive();
      if (r->h.dl != 0x00) Drive[0] = r->h.dl + 'A' - 1;
      fd32_getcwd(Drive, Cwd);
      strcpy(Dest, Cwd + 1); /* Skip leading backslash as required from DOS */
      return;
    }

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
        r->x.ax = (WORD) Res;
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
                        (char *) (r->x.es << 4) + r->x.di);
      dos_return(Res, r);
      return;
      
    /* Truename */
    case 0x60:
      /* Only subservice 01h "Get short (8.3) filename for file" is implemented */
      if (r->h.cl != 0x01)
      {
        dos_return(FD32_EINVAL, r);
        return;
      }
      /* CH    SUBST expansion flag (ignored): 80h to not resolve SUBST */
      /* DS:SI pointer to the ASCIZ long filename or path               */
      /* ES:DI pointer to a buffer for short pathname                   */
      Res = fd32_sfn_truename((char *) (r->x.es << 4) + r->x.di,
                              (char *) (r->x.ds << 4) + r->x.si);
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
                      r->x.di, &Action);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res;    /* The new handle */
      r->x.cx = (WORD) Action; /* Action taken   */
      return;
    }

    /* Get volume informations */
    case 0xA0:
    {
      fd32_fs_info_t FSInfo;
      /* DS:DX pointer to the ASCIZ root directory name          */
      /* ES:DI pointer to a buffer to store the file system name */
      /* CX    size of the buffer pointed by ES:DI               */
      FSInfo.Size       = sizeof(fd32_fs_info_t);
      FSInfo.FSNameSize = r->x.cx;
      FSInfo.FSName     = (char *) (r->x.es << 4) + r->x.di;
      Res = fd32_get_fsinfo((char *) (r->x.ds << 4) + r->x.dx, &FSInfo);
      r->x.bx = FSInfo.Flags;
      r->x.cx = FSInfo.NameMax;
      r->x.dx = FSInfo.PathMax;
      dos_return(Res, r);
      return;
    }

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
  int      Res;
  LONGLONG llRes;
  char     SfnPath[FD32_LFNPMAX];

  switch (r->h.ah)
  {
    /* DOS 1+ - Direct character input, without echo */
    case 0x07:
    {
      /* Perform 1-byte read from the stdin, file handle 0. */
      /* This call doesn't check for Ctrl-C or Ctrl-Break.  */
      char Ch;
      #if 1
      fd32_read_t R;
      /* TODO: Console input is not redirectable with the current approach */
      /* Set up support for direct character input via keyboard */
      if (kbdreq == NULL)
      {
        if ((Res = fd32_dev_search("kbd")) < 0)
        {
          LOG_PRINTF(("Unable to find the keyboard device. Direct character input disabled\n"));
          return;
        }
        fd32_dev_get(Res, &kbdreq, &R.DeviceId, NULL, 0);
      }
      R.Size        = sizeof(fd32_read_t);
      R.Buffer      = &Ch;
      R.BufferBytes = 1;
      Res = kbdreq(FD32_READ, &R);
      #else
      Res = fd32_read(0, &Ch, 1);
      #endif
      if (Res >= 0) r->h.al = Ch; /* Return the character in AL */
      /* TODO: from RBIL:
         if the interim console flag is set (see AX=6301h), partially-formed
         double-byte characters may be returned */
      return;
    }

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
      LOG_PRINTF(("INT 21h - Getting default drive\n"));
      /* Return the default drive in AL (0='A', etc.) */
      r->h.al = fd32_get_default_drive() - 'A';
      RMREGS_CLEAR_CARRY;
      return;

    /* DOS 1+ - Set Disk Transfer Area address */
    case 0x1A:
      /* DS:DX pointer to the new DTA */
      current_psp->dta = (void *) (r->x.ds << 4) + r->x.dx;
      return;

    /* DOS 2+ - Get Disk Transfer Area address */
    case 0x2F:
      /* ES:BX pointer to the current DTA */
      /* FIX ME: I assumed low memory DTA, I'm probably wrong! */
      r->x.es = (DWORD) current_psp->dta >> 4;
      r->x.bx = (DWORD) current_psp->dta & 0xF;
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
      /* TODO: MS-DOS returns AL=FFh for invalid subfunctions.     */
      /*       Make this behaviour consistent with other services. */
      r->h.al = 0xFF;
      RMREGS_SET_CARRY;
      return;
      
    /* DOS 2+ - Get free disk space */
    case 0x36:
    {
      /* DL    drive number (00h = default, 01h = A:, etc) */
      char             DriveSpec[4] = " :\\";
      fd32_getfsfree_t FSFree;
      DriveSpec[0] = fd32_get_default_drive();
      if (r->h.dl != 0x00) DriveSpec[0] = r->h.dl + 'A' - 1;
      FSFree.Size = sizeof(fd32_getfsfree_t);
      Res = fd32_get_fsfree(DriveSpec, &FSFree);
      LOG_PRINTF(("INT 21h - Getting free disk space for \"%s\", returns %08x\n", DriveSpec, Res));
      LOG_PRINTF(("          Got: %lu secperclus, %lu availclus, %lu bytspersec, %lu totclus\n",
                  FSFree.SecPerClus, FSFree.AvailClus, FSFree.BytesPerSec, FSFree.TotalClus));
      if (Res < 0)
      {
        r->x.ax = 0xFFFF; /* Unusual way to report error condition */
        return;
      }
      r->x.ax = FSFree.SecPerClus;  /* AX = Sectors per clusters */
      r->x.bx = FSFree.AvailClus;   /* BX = Number of free clusters */
      r->x.cx = FSFree.BytesPerSec; /* CX = Bytes per sector        */
      r->x.dx = FSFree.TotalClus;   /* DX = Total clusters on drive */
      return;
    }

    /* DOS 2+ - "MKDIR" - Create subdirectory */
    case 0x39:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = make_sfn_path(SfnPath, (char *) (r->x.ds << 4) + r->x.dx);
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_mkdir(SfnPath);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "RMDIR" - Remove subdirectory */
    case 0x3A:
      /* DS:DX pointer to the ASCIZ directory name */
      Res = make_sfn_path(SfnPath, (char *) (r->x.ds << 4) + r->x.dx);
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_rmdir(SfnPath);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "CREAT" - Create or truncate file */
    case 0x3C:
      /* DS:DX pointer to the ASCIZ file name               */
      /* CX    attribute mask for the new file              */
      /*       (volume label and directory are not allowed) */
      Res = make_sfn_path(SfnPath, (char *) (r->x.ds << 4) + r->x.dx);
      dos_return(Res, r);
      LOG_PRINTF(("INT 21h - Creat \"%s\" with attr %04x\n", SfnPath, r->x.cx));
      if (Res < 0) return;
      Res = fd32_open(SfnPath,
                      FD32_ORDWR | FD32_OCOMPAT | FD32_OTRUNC | FD32_OCREAT,
                      r->x.cx,
                      0,   /* alias hint, not used   */
                      NULL /* action taken, not used */);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* The new handle */
      return;

    /* DOS 2+ - "OPEN" - Open existing file */
    case 0x3D:
      /* DS:DX pointer to the ASCIZ file name                 */
      /* AL    opening mode                                   */
      /* CL    attribute mask for to look for (?server call?) */
      Res = make_sfn_path(SfnPath, (char *) (r->x.ds << 4) + r->x.dx);
      dos_return(Res, r);
      LOG_PRINTF(("INT 21h - Open \"%s\" with mode %02x\n", SfnPath, r->h.al));
      if (Res < 0) return;
      Res = fd32_open(SfnPath, FD32_OEXIST | r->h.al, FD32_ANONE,
                      0,   /* alias hint, not used   */
                      NULL /* action taken, not used */);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* The new handle */
      return;

    /* DOS 2+ - "CLOSE" - Close file */
    case 0x3E:
      /* BX file handle */
      LOG_PRINTF(("INT 21h - Close handle %04x\n", r->x.bx));
      Res = fd32_close(r->x.bx);
      dos_return(Res, r); /* Recent DOSes preserve AH, that's OK for us */
      return;

    /* DOS 2+ - "READ" - Read from file or device */
    case 0x3F:
      /* BX    file handle                    */
      /* CX    number of bytes to read        */
      /* DS:DX pointer to the transfer buffer */
      Res = fd32_read(r->x.bx, (void *) (r->x.ds << 4) + r->x.dx, r->x.cx);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      #if 0
      /* This is a quick'n'dirty hack to return \n after \r from stdin.  */
      /* It works only if console input is on handle 0 and if the number */
      /* of bytes to read is greater than the first carriage return.     */
      /* TODO: Make this \r\n stuff functional according to the console. */
      if (r->x.bx == 0)
      {
        BYTE *Buf = (BYTE *) (r->x.ds << 4) + r->x.dx;
        if (Res < r->x.cx) Buf[Res++] = '\n';
      }
      #endif
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* Return the number of bytes read in AX */
      return;

    /* DOS 2+ - "WRITE" - Write to file or device */
    case 0x40:
      /* BX    file handle                    */
      /* CX    number of bytes to write       */
      /* DS:DX pointer to the transfer buffer */
      Res = fd32_write(r->x.bx, (void *) (r->x.ds << 4) + r->x.dx, r->x.cx);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res; /* Return the number of bytes written in AX */
      return;

    /* DOS 2+ - "UNLINK" - Delete file */
    case 0x41:
      /* DS:DX pointer to the ASCIZ file name                  */
      /* CL    attribute mask for deletion (?server call?)     */
      /*       FIX ME: check if they are required or allowable */
      Res = make_sfn_path(SfnPath, (char *) (r->x.ds << 4) + r->x.dx);
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_unlink(SfnPath, FD32_FAALL | FD32_FRNONE);
      dos_return(Res, r); /* In DOS 3.3 AL seems to be the file's drive */
      return;

    /* DOS 2+ - "LSEEK" - Set current file position */
    case 0x42:
      /* BX    file handle */
      /* CX:DX offset      */
      /* AL    origin      */
      Res = fd32_lseek(r->x.bx, (LONGLONG) (r->x.cx << 8) + r->x.dx,
                       r->h.al, &llRes);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      /* Return the new position from start of file in DX:AX */
      r->x.ax = (WORD) llRes;
      r->x.dx = (WORD) (llRes >> 16);
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
      r->x.ax = (WORD) FD32_EBADF;
      RMREGS_SET_CARRY;
      return;

    /* DOS 2+ - "DUP" - Duplicate file handle */
    case 0x45:
      /* BX file handle */
      Res = fd32_dup(r->x.bx);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
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

    /* DOS 2+ - "CWD" - Get current directory */
    case 0x47:
    {
      /* DL    drive number (00h = default, 01h = A:, etc) */
      /* DS:SI 64-byte buffer for ASCIZ pathname           */
      char  Drive[2] = "\0\0";
      char  Cwd[FD32_LFNPMAX];
      char *Dest = (char *) (r->x.ds << 4) + r->x.si;

      LOG_PRINTF(("INT 21h - Getting current dir of drive %02x\n", r->h.dl));
      Drive[0] = fd32_get_default_drive();
      if (r->h.dl != 0x00) Drive[0] = r->h.dl + 'A' - 1;
      fd32_getcwd(Drive, Cwd);
      fd32_sfn_truename(Cwd, Cwd); /* Convert all component to 8.3 format */
      strcpy(Dest, Cwd + 1); /* Skip leading backslash as required from DOS */
      r->x.ax = 0x0100; /* Undocumented success return value, from RBIL */
      return;
    }

    /* DOS 2+ - "EXEC" - Load and/or Execute program */
    case 0x4B:
    {
      tExecParams *pb;
      pb = (tExecParams *) ((r->x.es << 4) + r->x.bx);
      /* Only AH = 0x00 - Load and Execute - is currently supported */
      if (r->h.al != 0)
      {
        dos_return(FD32_EINVAL, r);
        return;
      }
      Res = dos_exec((char *) ((r->x.ds << 4) + r->x.dx),
                     pb->Env, pb->CmdTail, pb->Fcb1, pb->Fcb2,
                     &DosReturnCode);
    }
    dos_return(Res, r);
    return;
    
    /* DOS 2+ - Get return code */
    case 0x4D:
      RMREGS_CLEAR_CARRY;
      r->h.ah = 0; /* Only normal termination, for now...*/
      r->h.al = DosReturnCode;
      DosReturnCode = 0; /* DOS clears this after reading it */
      return;

    /* DOS 2+ - "FINDFIRST" - Find first matching file */
    case 0x4E:
      /* AL    special flag for APPEND (ignored)                   */
      /* CX    allowable attributes (archive and readonly ignored) */
      /* DS:DX pointer to the ASCIZ file specification             */
      LOG_PRINTF(("INT 21h - Findfirst \"%s\" with attr %04x\n", (char *) (r->x.ds << 4) + r->x.dx, r->x.cx));
      Res = fd32_dos_findfirst((char *) (r->x.ds << 4) + r->x.dx,
                               r->x.cx, current_psp->dta);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "FINDNEXT" - Find next matching file */
    case 0x4F:
      LOG_PRINTF(("INT 21h - Findnext\n"));
      Res = fd32_dos_findnext(current_psp->dta);
      dos_return(Res, r);
      return;

    /* DOS 2+ - "RENAME" - Rename or move file */
    case 0x56:
    {
      char SfnPathNew[FD32_LFNPMAX];
      /* DS:DX pointer to the ASCIZ name of the file to be renamed */
      /* ES:DI pointer to the ASCIZ new name for the file          */
      /* CL    attribute mask for server call (ignored)            */
      Res = make_sfn_path(SfnPath, (char *) (r->x.ds << 4) + r->x.dx);
      dos_return(Res, r);
      if (Res < 0) return;
      Res = make_sfn_path(SfnPathNew, (char *) (r->x.es << 4) + r->x.di);
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_rename(SfnPath, SfnPathNew);
      dos_return(Res, r);
      return;
    }

    /* DOS 2+ - Get/set file's time stamps */
    case 0x57:
      get_and_set_time_stamps(r);
      return;

    /* DOS 3.0+ - Create new file */
    case 0x5B:
      /* CX    attribute mask for the new file */
      /* DS:DX pointer to the ASCIZ file name  */
      Res = make_sfn_path(SfnPath, (char *) (r->x.ds << 4) + r->x.dx);
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_open(SfnPath, FD32_ORDWR | FD32_OCOMPAT | FD32_OCREAT,
                      r->x.cx, 0, NULL);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
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
      Res = make_sfn_path(SfnPath, (char *) (r->x.ds << 4) + r->x.si);
      dos_return(Res, r);
      if (Res < 0) return;
      Res = fd32_open(SfnPath, (DWORD) (r->x.dx << 16) + r->x.bx,
                      r->x.cx, 0, &Action);
      if (Res < 0)
      {
        RMREGS_SET_CARRY;
        r->x.ax = (WORD) Res;
        return;
      }
      RMREGS_CLEAR_CARRY;
      r->x.ax = (WORD) Res;    /* The new handle */
      r->x.cx = (WORD) Action; /* Action taken   */
      return;
    }

    /* Long File Name functions */
    case 0x71:
      if (use_lfn) {
	lfn_functions(r);
      } else {
        r->x.ax = 0x7100;
        RMREGS_SET_CARRY;
      }
      return;

    /* Windows 95 beta - "FindClose" - Terminate directory search */
    case 0x72:
      /* BX directory handle (FIX ME: This is not documented anywhere!) */
      Res = fd32_lfn_findclose(r->x.bx);
      dos_return(Res, r);
      return;
      
    /* Windows 95 FAT32 services */
    /* By now only AX=7303h "Get extended free space on drive" is supported */
    case 0x73:
    {
      tExtDiskFree     *Edf;
      fd32_getfsfree_t  FSFree;
      if (r->h.al != 0x03)
      {
        r->h.al = 0xFF; /* Unimplemented subfunction */
        RMREGS_SET_CARRY;
      }
      /* DS:DX ASCIZ string for drive ("C:\" or "\\SERVER\Share")      */
      /* ES:DI Buffer for extended free space structure (tExtFreSpace) */
      /* CX    Length of buffer for extended free space                */
      if (r->x.cx < sizeof(tExtDiskFree))
      {
        dos_return(FD32_EFORMAT, r);
        return;
      }
      Edf = (tExtDiskFree *) (r->x.es << 4) + r->x.di;
      if (Edf->Version != 0x0000)
      {
        dos_return(FD32_EFORMAT, r);
        return;
      }
      FSFree.Size = sizeof(fd32_getfsfree_t);
      Res = fd32_get_fsfree((char *) (r->x.ds << 4) + r->x.dx, &FSFree);
      if (Res >= 0)
      {
        Edf->Size = sizeof(tExtDiskFree);
        /* Currently we don't support volume compression */
        Edf->SecPerClus  = FSFree.SecPerClus;
        Edf->BytesPerSec = FSFree.BytesPerSec;
        Edf->AvailClus   = FSFree.AvailClus;
        Edf->TotalClus   = FSFree.TotalClus;
        Edf->RealSecPerClus  = FSFree.SecPerClus;
        Edf->RealBytesPerSec = FSFree.BytesPerSec;
        Edf->RealAvailClus   = FSFree.AvailClus;
        Edf->RealTotalClus   = FSFree.TotalClus;
      }
      dos_return(Res, r);
      return;
    }

    /* Unsupported or invalid functions */
    default:
      RMREGS_SET_CARRY;
      r->x.ax = (WORD) FD32_EINVAL;
      return;
  }
}
