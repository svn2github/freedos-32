/**************************************************************************
 * FreeDOS32 File System Layer                                            *
 * Wrappers for file system driver functions and JFT support              *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2002-2005, Salvatore Isaja                               *
 *                                                                        *
 * This is "truename.c" - Services to work with canonicalized file names  *
 *                        (setting and resolving current directory,       *
 *                        SUBSTed drives and relative paths).             *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 File System Layer (the SOFTWARE).   *
 *                                                                        *
 * The SOFTWARE is free software; you can redistribute it and/or modify   *
 * it under the terms of the GNU General Public License as published by   *
 * the Free Software Foundation; either version 2 of the License, or (at  *
 * your option) any later version.                                        *
 *                                                                        *
 * The SOFTWARE is distributed in the hope that it will be useful, but    *
 * WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the SOFTWARE; see the file GPL.txt;                         *
 * if not, write to the Free Software Foundation, Inc.,                   *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/

#include <ll/i386/hw-data.h>
#include <ll/i386/stdlib.h>
#include <ll/i386/string.h>

#include <kmem.h>
#include <filesys.h>
#include <errno.h>
#include <kernel.h> /* strcasecmp */

/* Define the __DEBUG__ symbol in order to activate log output */
//#define __DEBUG__
#ifdef __DEBUG__
 #define LOG_PRINTF(s) fd32_log_printf s
#else
 #define LOG_PRINTF(s)
#endif

#define ENABLE_SUBST 1


/* Current Directury Structure.                                               */
/* Each process owns a list of CDSs with the current directory for each drive */
typedef struct Cds
{
  struct Cds *Next;
  char        Drive[FD32_LFNPMAX];  /* Any valid drive letter or device name. */
  char        CurDir[FD32_LFNPMAX]; /* Canonicalized current directory,       */
                                    /* without drive specification but with   */
                                    /* initial backslash. Trailing backslash  */
                                    /* may or not be present. Example:        */
                                    /* "\dir1\dir2"                           */
}
tCds;


/* Implemented by the kernel, gets the CDS list head from the PSP */
extern void **fd32_get_cdslist();


#if ENABLE_SUBST
/* SUBSTituted drives structure.                                          */
/* The File System Layer owns a list of SUBST structures with the SUBSTed */
/* path for each drive.                                                   */
typedef struct Subst
{
  struct Subst *Next;
  /* DriveAlias can be any drive letter or any name valid as device name */
  char          DriveAlias[FD32_LFNPMAX];
  /* Target must be truenamed, trailing '\' may or not be present */
  char          Target[FD32_LFNPMAX];
}
tSubst;
/* The SUBST structures list head */
static tSubst *SubstList = NULL;


/* The CREATE SUBST system call.                               */
/* Puts a new drive alias on top of the SUBST structures list. */
/* Returns 0 on success, or a negative error code on failure.  */
int fd32_create_subst(char *DriveAlias, char *Target)
{
  int              Res;
  char            *Path;
  fd32_request_t  *request;
  void            *DeviceId;
  tSubst          *S;
  fd32_openfile_t  Of;
  fd32_close_t     C;

  for (;;)
  {
    /* TODO: The specified Target should be probably TRUENAMEd first */
    /* Check if the specified target drive exists */
    if ((Res = fd32_get_drive(Target, &request, &DeviceId, &Path)) < 0)
      return Res;
    /* Check if the specified target path exists and is valid */
    Of.Size     = sizeof(fd32_openfile_t);
    Of.DeviceId = DeviceId;
    Of.FileName = Path;
    Of.Mode     = FD32_OREAD | FD32_OEXIST | FD32_ODIR;
    /* Open file in Of.FileId */
    Res = request(FD32_OPENFILE, &Of);
    if (Res < 0)
    {
      if (Res == -ENOTMOUNT) continue;
      return Res;
    }
    C.Size     = sizeof(fd32_close_t);
    C.DeviceId = Of.FileId;
    if ((Res = request(FD32_CLOSE, &C)) < 0) return Res;
    /* Initialize a new tSubst structure and make it the SubstList head */
    if ((S = (void *)mem_get(sizeof(tSubst))) == NULL) return -ENOMEM;
    S->Next = SubstList;
    strcpy(S->DriveAlias, DriveAlias);
    strcpy(S->Target, Target);
    SubstList = S;
    return 0;
  }
}


/* The TERMINATE SUBST system call.                      */
/* Removes a drive alias from the SUBST structures list. */
/* Returns 0 on success, or -ENODEV if not found.        */
int fd32_terminate_subst(char *DriveAlias)
{
  tSubst *S, *PrevS;
  for (PrevS = NULL, S = SubstList; S; PrevS = S, S = S->Next)
    if (strcasecmp(S->DriveAlias, DriveAlias) == 0)
    {
      if (PrevS) PrevS->Next = S->Next;
            else SubstList   = S->Next;
      mem_free((DWORD)S, sizeof(tSubst));
      return 0;
    }
  return -ENODEV;
}


/* The QUERY SUBST system call.                                */
/* Gets the actual path associated with a SUBSTed drive alias. */
/* On success, returns 0 and fills the Target string.          */
/* If the drive alias is not found returns -ENODEV.            */
int fd32_query_subst(char *DriveAlias, char *Target)
{
  tSubst *S;
  for (S = SubstList; S; S = S->Next)
    if (strcasecmp(S->DriveAlias, DriveAlias) == 0)
    {
      strcpy(Target, S->Target);
      return 0;
    }
  return -ENODEV;
}
#endif /* #if ENABLE_SUBST */


/* The CHDIR system call.                                            */
/* Sets the current directory of the specified logical drive for the */
/* current process to the specified directory, that must be valid.   */
/* Returns 0 on success, or a negative error code on failure.        */
int fd32_chdir(/*const */char *DirName)
{
  int               Res;
  char              Aux[FD32_LFNPMAX];
  char              Drive[FD32_LFNMAX];
  char             *Path;
  fd32_request_t   *request;
  void             *DeviceId;
  fd32_openfile_t   Of;
  fd32_close_t      C;
  tCds             *D;
  tCds            **CdsList = (tCds **) fd32_get_cdslist();

  LOG_PRINTF(("[CHDIR] In:\"%s\"\n", DirName));
  /* Open the directory to check if it is a valid directory */
  if ((Res = fd32_truename(Aux, DirName, FD32_TNSUBST)) < 0) return Res;
  for (;;)
  {
    Res = fd32_get_drive(Aux, &request, &DeviceId, &Path);
    if (Res < 0) return Res;
    Of.Size     = sizeof(fd32_openfile_t);
    Of.DeviceId = DeviceId;
    Of.FileName = Path;
    Of.Mode     = FD32_OREAD | FD32_OEXIST | FD32_ODIR;
    Res = request(FD32_OPENFILE, &Of);
    if (Res == FD32_OROPEN) break;
    if (Res != -ENOTMOUNT) return Res;
  }
  /* If we arrive here, the directory is valid */
  C.Size     = sizeof(fd32_close_t);
  C.DeviceId = Of.FileId;
  request(FD32_CLOSE, &C);
  if ((Res = fd32_truename(Aux, DirName, FD32_TNDOTS)) < 0) return Res;
  for (Res = 0; Aux[Res] != ':'; Drive[Res] = Aux[Res], Res++);
  Drive[Res] = 0;

  LOG_PRINTF(("[CHDIR] Setting the current dir of \"%s\" to \"%s\"\n", Drive, &Aux[Res + 1]));

  /* Search for the specified drive in the CDS list of the current process */
  for (D = *CdsList; D; D = D->Next)
    if (strcasecmp(D->Drive, Drive) == 0)
    {
      strcpy(D->CurDir, &Aux[Res + 1]);
      break;
    }

  /* If no CDS is present for the specified drive, add the entry */
  if ((D = (void *)mem_get(sizeof(tCds))) == NULL) return -ENOMEM;
  D->Next = *CdsList;
  strcpy(D->Drive, Drive);
  strcpy(D->CurDir, &Aux[Res + 1]);
  *CdsList = D;
  return 0;
}


#if 0
/* Returns nonzero if a drive specifications appears to be a valid drive. */
/* Called by fd32_truename.                                               */
static int is_a_valid_drive(char *Drive)
{
  int Letter;

  /* If it's a 1-byte drive, check if it's a valid drive letter */
  if (Drive[1] == 0)
  {
    Letter = toupper(Drive[0]);
    if ((Letter < 'A') || (Letter > 'Z')) return 0;
    if (!(Drives[Letter - 'A'])) return 0;
    return 1;
  }
  /* Otherwise search for a device with the specified name */
  if (!(fd32_dev_find(Drive))) return 0;
  return 1;
}
#endif


/* Gets the current directory of a drive for the current process. */
/* Called by fd32_truename.                                       */
/* TODO: Should return "invalid drive" on error. */
int fd32_getcwd(const char *Drive, char *Dest)
{
  tCds  *C;
  tCds **CdsList = (tCds **) fd32_get_cdslist();

  /* Search for the specified drive in the CDS list of the current process */
  for (C = *CdsList; C; C = C->Next)
    if (strcasecmp(C->Drive, Drive) == 0)
    {
      strcpy(Dest, C->CurDir);
      LOG_PRINTF(("[GETCWD] Getting the current dir of \"%s\": \"%s\"\n", Drive, Dest));
      return 0;
    }
  strcpy(Dest, "\\");
  LOG_PRINTF(("[GETCWD] No CDS for \"%s\", getting root\n", Drive));
  return 0;
}


/* Canonicalizes a path name.                                 */
/* Returns 0 on success, or a negative error code on failure. */
/* TODO: truename for long names with verification */
/* TODO: make Source const char */
int fd32_truename(char *Dest, char *Source, DWORD Flags)
{
  char    Aux[FD32_LFNPMAX];   /* Internal destination buffer */
  char    Drive[FD32_LFNPMAX]; /* Stores the original drive   */
  char    Comp[FD32_LFNMAX];   /* Stores each path component  */
  char   *s;
  char   *pAux;           /* Pointer into the Aux string   */
  char   *pDrive = Drive; /* Pointer into the Drive string */
  char   *pComp;          /* Pointer into the Comp string  */
  char   *RootStart;
  int     Dots;
  #if ENABLE_SUBST
  tSubst *S;
  #endif

  LOG_PRINTF(("[TRUENAME] In:\"%s\"\n", Source));
  /* Get the drive specification */
  for (s = Source; *Source && (*Source != ':'); *(pDrive++) = *(Source++));
  *pDrive = 0;
  if (!(*Source))
  {
    Drive[0] = fd32_get_default_drive();
    Drive[1] = 0;
    Source   = s;
  }
  else Source++; /* Skip ':' */
  /* If Drive is not a SUBSTed drive, copy it to the destination */
  for (pAux = Aux, s = Drive; *s; *pAux++ = *s++); *pAux++ = ':';
  /* Check if Drive refers to a SUBSTed drive alias */
  #if ENABLE_SUBST
  if (Flags & FD32_TNSUBST)
    for (S = SubstList; S; S = S->Next)
      if (strcasecmp(S->DriveAlias, Drive) == 0)
      {
        /* If Drive is an alias, copy the target to the destination, */
        /* removing the trailing '\' if present.                     */
        for (pAux = Aux, s = S->Target; *s; *pAux++ = *s++);
        if (*(pAux - 1) == '\\') pAux--;
        break;
      }
  #endif
//  if (!(S || is_a_valid_drive(Drive))) return -ENODEV;
  /* The root directory in the destination starts right after the Drive.  */
  /* If Drive was a SUBSTed drive, the root start right after the target. */
  RootStart = pAux;
  /* If a relative path has been specified, append the current directory */
  /* and the trailing '\' if not present, otherwise just append a '\'.   */
  if ((*Source != '\\') && (*Source != '/'))
  {
    fd32_getcwd(Drive, Comp);
    for (pComp = Comp; (*pAux = *pComp); pAux++, pComp++);
    if (*(pAux - 1) != '\\') *pAux++ = '\\';
  }
  else
  {
    *(pAux++) = '\\';
    Source++;
  }
  /* Canonicalize the rest of the pathname */
  while (*Source)
  {
    /* Extract the next partial from the path (before '\' or '/' or NULL) */
    pComp = Comp;
    while ((*Source != '\\') && (*Source != '/') && (*Source))
      *(pComp++) = *(Source++);
    if (*Source) Source++;
    *pComp = 0;
    /* Skip consecutive slashes */
    if (pComp == Comp) continue;
    
    LOG_PRINTF(("[TRUENAME] Comp:\"%s\"\n", Comp));

    /* Search for ".", "..", "..." etc. entries */
    Dots = 0;
    for (pComp = Comp; *pComp; pComp++)
      if (*pComp == '.') Dots++;
                    else { Dots = 0; break; }

    /* If not a ".", "..", "..." etc. entry, append the Partial to Aux */
    if (Dots == 0)
    {
      for (pComp = Comp; (*pAux = *pComp); pAux++, pComp++);
      *pAux++ = '\\';
    }
    else /* process the dot entries as requested */
    {
      if (Flags & FD32_TNDOTS)
      {
        /* rewind the path in Aux for the number of dots minus one */
        if (Dots > 1) for (;;)
        {
          if (*(pAux - 1) == '\\') Dots--;
          if (Dots == 0) break;
          if (--pAux == RootStart) return -ENOTDIR;
        }
      }
      else
      {
        /* Replace dot sequences with the appropriate number of "..\" */
        for (Dots--; Dots; Dots--)
        {
          *pAux++ = '.';
          *pAux++ = '.';
          *pAux++ = '\\';
        }
      }
    }
  }
  /* Remove trailing backslash if present, add the null */
  /* terminator and copy the Aux buffer to Dest         */
  if ((*(pAux - 1) == '\\') && (*(pAux - 2) != ':')) pAux--;
  *pAux = 0;
  strcpy(Dest, Aux);
  LOG_PRINTF(("[TRUENAME] Out:\"%s\"\n", Dest));
  return 0;
}


/* TODO: make Source const char* (by now called functions would complain) */
/* TODO: SUBSTed drives are always resolved, make this selectable         */
/* TODO: There's a bug here, p is never reassigned to Partial, but I'm too lazy to think on it now... */
int fd32_sfn_truename(char *Dest, char *Source)
{
  int                Res;
  char               Partial[FD32_LFNPMAX];
  char               Aux[FD32_LFNPMAX];
  char              *a = Aux;
  char              *d = Dest;
  char              *p = Partial;
  char              *n;
  fd32_fs_lfnfind_t  F;

  LOG_PRINTF(("[SFN_TRUENAME] fd32_sfn_truename, Source=\"%s\"\n", Source));
  if ((Res = fd32_truename(Aux, Source, FD32_TNSUBST)) < 0) return Res;
  /* Copy drive specification */
  for (; *a != '\\'; p++, d++, a++)
  {
    *d = *a;
    *p = *a;
  }
  while (*a)
  {
    *(d++) = *a;
    *(p++) = *(a++);
    *d = 0;
    if (*a == 0) break;
    while ((*a != '\\') && *a) *(p++) = *(a++);
    *p = 0;
    LOG_PRINTF(("[SFN_TRUENAME] Searching \"%s\"\n", Partial));
    if ((Res = fd32_lfn_findfirst(Partial, FD32_FRNONE | FD32_FAALL, &F)) < 0) return Res;
    if (Res < 0) return Res;
    if ((Res = fd32_lfn_findclose(Res)) < 0) return Res;
    for (n = F.ShortName; (*d = *n); d++, n++);
    *d = 0;
    LOG_PRINTF(("[SFN_TRUENAME] Dest=\"%s\"\n", Dest));
  }
  return 0;
}
