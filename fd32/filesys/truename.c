#include <dr-env.h>

#include <filesys.h>
#include <unicode.h>
#include <errors.h>


/* Current Directury Structure.                                               */
/* Each process owns a list of CDSs with the current directory for each drive */
typedef struct Cds
{
  struct Cds *Next;
  /* Drive can be any drive letter or any name valid as device name */
  char        Drive[FD32_LFNPMAX];
  /* CurDir must be truenamed, trailing '\' may or not be present */
  char        CurDir[FD32_LFNPMAX];
}
tCds;


/* Implemented by the kernel, gets the CDS list head from the PSP */
extern void **fd32_get_cdslist();


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
      if (Res == FD32_ENMOUNT) continue;
      return Res;
    }
    C.Size     = sizeof(fd32_close_t);
    C.DeviceId = Of.FileId;
    if ((Res = request(FD32_CLOSE, &C)) < 0) return Res;
    /* Initialize a new tSubst structure and make it the SubstList head */
    if ((S = fd32_kmem_get(sizeof(tSubst))) == NULL) return FD32_ENOMEM;
    S->Next = SubstList;
    strcpy(S->DriveAlias, DriveAlias);
    strcpy(S->Target, Target);
    SubstList = S;
    return 0;
  }
}


/* The TERMINATE SUBST system call.                      */
/* Removes a drive alias from the SUBST structures list. */
/* Returns 0 on success, or FD32_ENODRV if not found.    */
int fd32_terminate_subst(char *DriveAlias)
{
  tSubst *S, *PrevS;
  for (PrevS = NULL, S = SubstList; S; PrevS = S, S = S->Next)
    if (utf8_stricmp(S->DriveAlias, DriveAlias) == 0)
    {
      if (PrevS) PrevS->Next = S->Next;
            else SubstList   = S->Next;
      fd32_kmem_free(S, sizeof(tSubst));
      return 0;
    }
  return FD32_ENODRV;
}


/* The QUERY SUBST system call.                                */
/* Gets the actual path associated with a SUBSTed drive alias. */
/* On success, returns 0 and fills the Target string.          */
/* If the drive alias is not found returns FD32_ENODRV.        */
int fd32_query_subst(char *DriveAlias, char *Target)
{
  tSubst *S;
  for (S = SubstList; S; S = S->Next)
    if (utf8_stricmp(S->DriveAlias, DriveAlias) == 0)
    {
      strcpy(Target, S->Target);
      return 0;
    }
  return FD32_ENODRV;
}


/* The CHDIR system call.                                            */
/* Sets the current directory of the specified logical drive for the */
/* current process to the specified directory, that must be valid.   */
/* Returns 0 on success, or a negative error code on failure.        */
int fd32_chdir(char *DirName)
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
    if (Res == 0) break;
    if (Res != FD32_ENMOUNT) return Res;
  }
  /* If we arrive here, the directory is valid */
  C.Size     = sizeof(fd32_close_t);
  C.DeviceId = Of.FileId;
  request(FD32_CLOSE, &C);
  if ((Res = fd32_truename(Aux, DirName, FD32_TNDOTS)) < 0) return Res;
  for (Res = 0; Aux[Res] != ':'; Drive[Res] = Aux[Res], Res++);
  Drive[Res] = 0;

  #ifdef DEBUG
  fd32_log_printf("Setting the current dir of %s to %s\n", Drive, &Aux[Res + 1]);
  #endif

  /* Search for the specified drive in the CDS list of the current process */
  for (D = *CdsList; D; D = D->Next)
    if (utf8_stricmp(D->Drive, Drive) == 0)
    {
      strcpy(D->CurDir, &Aux[Res + 1]);
      break;
    }

  /* If no CDS is present for the specified drive, add the entry */
  if ((D = fd32_kmem_get(sizeof(tCds))) == NULL) return FD32_ENOMEM;
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
    if (utf8_stricmp(C->Drive, Drive) == 0)
    {
      strcpy(Dest, C->CurDir);
      return 0;
    }
  strcpy(Dest, "\\");
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
  tSubst *S;

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
  if (Flags & FD32_TNSUBST)
    for (S = SubstList; S; S = S->Next)
      if (utf8_stricmp(S->DriveAlias, Drive) == 0)
      {
        /* If Drive is an alias, copy the target to the destination, */
        /* removing the trailing '\' if present.                     */
        for (pAux = Aux, s = S->Target; *s; *pAux++ = *s++);
        if (*(pAux - 1) == '\\') pAux--;
        break;
      }
//  if (!(S || is_a_valid_drive(Drive))) return FD32_ERROR_UNKNOWN_UNIT;
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
    
    #ifdef DEBUG
    fd32_log_printf("Component : \"%s\"\n", Comp);
    #endif

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
          if (--pAux == RootStart) return FD32_ENOTDIR;
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
  if (*(pAux - 1) == '\\') pAux--;
  *pAux = 0;
  strcpy(Dest, Aux);
  return 0;
}


/* TODO: make Source const char */
/* TODO: doesn't work on SUBSTed drives */
/* TODO: There's a bug here, p is never reassigned to Partial, but I'm too lazy to think on it now... */
int fd32_sfn_truename(char *Dest, char *Source)
{
  int                Res, NumSlashes = 0;
  char               Partial[FD32_LFNPMAX];
  char               Aux[FD32_LFNPMAX];
  char              *a = Aux;
  char              *p = Partial;
  fd32_fs_lfnfind_t  F;

  if ((Res = fd32_truename(Aux, Source, FD32_TNSUBST)) < 0) return Res;
  while (*a)
  {
    if (*a == '\\')
    {
      *p = 0;
      NumSlashes++;
      if (NumSlashes == 1) strcpy(Dest, Partial);
      else
      {
        if ((Res = fd32_lfn_findfirst(Partial, FD32_FRNONE | FD32_FAALL, &F)) < 0) return Res;
        if (Res < 0) return Res;
        if ((Res = fd32_lfn_findclose(Res)) < 0) return Res;
        strcat(Dest, F.ShortName);
        strcat(Dest, "\\");
      }
    }
    *p++ = *a++;
  }
  if ((Res = fd32_lfn_findfirst(Partial, FD32_FRNONE | FD32_FAALL, &F)) < 0) return Res;
  if ((Res = fd32_lfn_findclose(Res)) < 0) return Res;
  strcat(Dest, F.ShortName);
  return 0;
}
