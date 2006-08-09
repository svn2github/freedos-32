/* Mini MSVCRT
 * by Hanzac Chen
 *
 * This is free software; see GPL.txt
 */


#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <locale.h>
#include <sys/stat.h>

#include "winb.h"

typedef  int (* _onexit_t)(void);

static _onexit_t fd32_imp__dllonexit(_onexit_t func, _onexit_t **start, _onexit_t **end)
{
  _onexit_t *tmp;
  int len;

#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] (%p,%p,%p)\n", func, start, end);
#endif

  if (!start || !*start || !end || !*end)
  {
    error("MSVCRT dllonexit got bad table!\n");
    return NULL;
  }

  len = (*end - *start);
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] table start %p-%p, %d entries\n", *start, *end, len);
#endif

  if (++len <= 0)
    return NULL;

  tmp = (_onexit_t *)realloc(*start, len * sizeof(tmp));
  if (!tmp)
    return NULL;
  *start = tmp;
  *end = tmp + len;
  tmp[len - 1] = func;
#ifdef __WINB_DEBUG__
  fd32_log_printf("[WINB] new table start %p-%p, %d entries\n", *start, *end, len);
#endif
  return func;
}

static void fd32_imp__set_app_type(int a)
{
  fd32_log_printf("[WINB] Set app type: %d\n", a);
}

static void fd32_imp_cexit(void)
{
  fd32_log_printf("[WINB] cexit (Do nothing)\n");
}

static uint32_t fd32_imp_onexit(void * func)
{
  fd32_log_printf("[WINB] onexit (func: %x)\n", (uint32_t)func);
  return 0;
}

static int fd32_imp_setmode(int fd, int mode)
{
  fd32_log_printf("[WINB] setmode (fd: %x mode: %x)\n", fd, mode);
  return 0;
}

/* _amsg_exit(rterrnum) - Fast exit fatal errors */
static void fd32_imp_amsg_exit(int errnum)
{
  fd32_log_printf("[WINB] Fast exit errnum: %d\n", errnum);
  exit(255);
}

static int fd32_imp_XcptFilter(int ex, uint32_t ptr)
{
  fd32_log_printf("[WINB] XcptFilter (ex: %x, ptr: %x\n)", ex, ptr);
  return 0;
}

typedef void (*_INITTERMFUN)(void);
static unsigned int fd32_imp_initterm(_INITTERMFUN *start,_INITTERMFUN *end)
{
  _INITTERMFUN* current = start;

  printf("(%p,%p)\n",start,end);
  while (current<end)
  {
    if (*current)
    {
      printf("Call init function %p\n", *current);
      (*current)();
      printf("returned\n");
    }
    current++;
  }
  return 0;
}

/* NOTE: Simple controlfp, why the WINE.MSVCRT._control87 is so complicated? */
static unsigned int fd32_imp_control87(unsigned int newval, unsigned int mask)
{
#if defined(__GNUC__) && defined(__i386__)
  unsigned int oldval = 0;

  fd32_log_printf("[WINB] _control87(%08x, %08x): Called\n", newval, mask);

  /* Get fp control word */
  __asm__ __volatile__( "fstcw %0" : "=m" (oldval) : );

  fd32_log_printf("[WINB] Control word before : %08x\n", oldval);

  /* Mask with parameters */
  newval = (oldval & ~mask) | (newval & mask);

  fd32_log_printf("[WINB] Control word after  : %08x\n", newval);

  /* Put fp control word */
  __asm__ __volatile__( "fldcw %0" : : "m" (newval) );

  return oldval&0x0FFFF;
#else
  fd32_log_printf("[WINB] controlfp Not Implemented on this target!\n");
  return 0;
#endif
}

static unsigned int fd32_imp_controlfp(unsigned int newval, unsigned int mask)
{
  return fd32_imp_control87(newval, mask);
}

static int fd32_imp_except_handler3(uint32_t rec, void* frame, uint32_t context, void* dispatcher)
{
  fd32_log_printf("[WINB] except_handler3 Not Implemented!\n");
  return 0;
}

static int fd32_imp_adjust_fdiv;

typedef int (*MSVCRT_matherr_func)(void *);
static MSVCRT_matherr_func fd32_imp_matherr_func = NULL;
static void fd32_imp__setusermatherr(MSVCRT_matherr_func func)
{
  fd32_imp_matherr_func = func;
  fd32_log_printf("[WINB] Set new matherr handler: %x\n", (uint32_t)func);
}

static unsigned int fd32_imp_commode;
static unsigned int* fd32_imp__p__commode(void)
{
  return &fd32_imp_commode;
}

static unsigned int fd32_imp_fmode;
static unsigned int* fd32_imp__p__fmode(void)
{
  return &fd32_imp_fmode;
}

/* from Newlib */
extern char ** environ;
static char *** fd32_imp__p___initenv(void)
{
  return &environ;
}

static char *** fd32_imp__p__environ(void)
{
  return &environ;
}

static int local_argc;
static char **local_argv_ptr = NULL;
static char *** fd32_imp__p___argv(void)
{
  return &local_argv_ptr;
}
static int * fd32_imp__p___argc(void)
{
  return &local_argc;
}

static void fd32_imp__getmainargs(int *argc, char** *argv, char** *envp, int expand_wildcards, int *new_mode)
{
  fd32_log_printf("[WINB] GETMAINARGS: %x, %x, %x, %x, %x\n", (uint32_t)argc, (uint32_t)argv, (uint32_t)envp, (uint32_t)expand_wildcards, (uint32_t)new_mode);
  fd32_log_printf("[WINB] Current process cmdline: %s\n", fd32_get_current_pi()->args);

  *argc = fd32_get_argv(fd32_get_current_pi()->filename, fd32_get_current_pi()->args, argv);
  local_argv_ptr = *argv;
}

/* Maximum number of bytes in multi-byte character in the current locale */
static int * fd32_imp__mb_cur_max(void)
{
  static int mb_cur_max = 1;
  /* NOTE: MT not supported */
  return &mb_cur_max;
}

/* TODO: check if it's correct */
static const char *fd32_imp_pctype = _ctype_ + 1;
/* TODO: support MB character */
static int fd32_imp_isctype(int c, int type)
{
  if (c >= -1 && c <= 255)
    return fd32_imp_pctype[c] & type;
  else
    return 0;
}

/* TODO: How to set these FILE descriptors to stdin, stdout and stderr */
typedef struct fd32_iobuf
{
  char*	_ptr;
  int	_cnt;
  char*	_base;
  int	_flag;
  int	_file;
  int	_charbuf;
  int	_bufsiz;
  char*	_tmpfname;
} fd32_FILE;
static fd32_FILE fd32_imp_iob[3] = {
  /* stdin  (fd32_imp_iob[0]) */
  { NULL, 0, NULL, 0, 0, 0, 0 },
  /* stdout (fd32_imp_iob[1]) */
  { NULL, 0, NULL, 0, 1, 0, 0 },
  /* stderr (fd32_imp_iob[3]) */
  { NULL, 0, NULL, 0, 2, 0, 0 },
};

/* NOTE: Wrap newlib's standard File stream functions to avoid the conflict of different stdin, stdout, stderr */
static FILE *newlib_stdfp(FILE *fp)
{
  if ((uint32_t)fp == (uint32_t)(fd32_imp_iob+0))
    fp = stdin;
  else if ((uint32_t)fp == (uint32_t)(fd32_imp_iob+1))
    fp = stdout;
  else if ((uint32_t)fp == (uint32_t)(fd32_imp_iob+2))
    fp = stderr;
  return fp;
}

static int fd32_imp$fflush(FILE *fp)
{
  return fflush(newlib_stdfp(fp));
}

static int fd32_imp$fprintf(FILE *fp, const char *fmt, ...)
{
  int ret;
  va_list ap;

  va_start(ap, fmt);
  ret = vfprintf(newlib_stdfp(fp), fmt, ap);
  va_end(ap);

  return ret;
}

static int fd32_imp$fputc(int c, FILE *fp)
{
  return fputc(c, newlib_stdfp(fp));
}

static int fd32_imp$fputs(const char *s, FILE *fp)
{
  return fputs(s, newlib_stdfp(fp));
}

static size_t fd32_imp$fread(void *data, size_t size, size_t count, FILE *fp)
{
  return fread(data, size, count, newlib_stdfp(fp));
}

static size_t fd32_imp$fwrite(const void *data, size_t size, size_t count, FILE *fp)
{
  return fwrite(data, size, count, newlib_stdfp(fp));
}

static int fd32_imp$vfprintf(FILE *fp, const char *fmt, va_list ap)
{
  return vfprintf(newlib_stdfp(fp), fmt, ap);
}

/* from Newlib */
extern int *__errno(void);
extern int __srefill(FILE * fp);

static int fd32_imp_filbuf(FILE * fp)
{
  int res = __srefill(newlib_stdfp(fp));
  
  if (res != EOF)
    res = getc(newlib_stdfp(fp));
  
  return res;
}

static char msvcrt_name[] = "msvcrt.dll";
static char msvcr7_name[] = "msvcr70.dll";
static struct symbol msvcrt_symarray[] = {
/* TODO */
/* The difference between _control87 and _controlfp is the way these two functions treat DENORMAL values. For Intel (x86) platforms, _control87 can set and clear the DENORMAL OPERAND exception mask. ALPHA platforms do not support this exception, and _controlfp does not modify the DENORMAL OPERAND exception mask. The following example demonstrates the difference:

_control87( _EM_INVALID, _MCW_EM ); // DENORMAL is unmasked by this call
_controlfp( _EM_INVALID, _MCW_EM ); // DENORMAL exception mask remains unchanged
*/
  {"_chdir",     0},
  {"_dup",     0},
  {"_dup2",     0},
  {"_getch",     0},
  {"_getcwd",     0},
  {"_mkdir",     0},
  {"_putch",     0},
  {"_putenv",     0},
  {"_rmdir",     0},
  {"_chdrive",     0},
  {"_clearfp",     0},
  {"_cprintf",     0},
  {"_cputs",     0},
  {"_findfirst",     0},
  {"_findnext",     0},
  {"_flsbuf",     0},
  {"_fullpath",     0},
  {"_futime",     0},
  {"_getdiskfree",     0},
  {"_getdrive",     0},
  {"_makepath",     0},
  {"_splitpath",     0},

  {"__dllonexit",     fd32_imp__dllonexit},
  {"__getmainargs",   fd32_imp__getmainargs},
  {"__mb_cur_max",    fd32_imp__mb_cur_max},
  {"__p___argc",      fd32_imp__p___argc},
  {"__p___argv",      fd32_imp__p___argv},
  {"__p___initenv",   fd32_imp__p___initenv},
  {"__p__commode",    fd32_imp__p__commode},
  {"__p__environ",    fd32_imp__p__environ},
  {"__p__fmode",      fd32_imp__p__fmode},
  {"__set_app_type",  fd32_imp__set_app_type},
  {"__setusermatherr",fd32_imp__setusermatherr},
  {"_adjust_fdiv",   &fd32_imp_adjust_fdiv},
  {"_amsg_exit",      fd32_imp_amsg_exit},
  {"_control87",      fd32_imp_control87},
  {"_controlfp",      fd32_imp_controlfp},
  {"_cexit",          fd32_imp_cexit},
  {"_except_handler3",fd32_imp_except_handler3},
  {"_filbuf",         fd32_imp_filbuf},
  {"_initterm",       fd32_imp_initterm},
  {"_iob",            fd32_imp_iob},
  {"_isctype",        fd32_imp_isctype},
  {"_onexit",         fd32_imp_onexit},
  {"_pctype",        &fd32_imp_pctype},
  {"_setmode",        fd32_imp_setmode},
  {"_XcptFilter",     fd32_imp_XcptFilter},

  /* from Newlib and libfd32 */
  {"_access",         access},
  {"_close",          close},
  {"_errno",          __errno},
  {"_exit",           exit},
  {"_fdopen",         fdopen},
  {"_fstat",          fstat},
  {"_isatty",         isatty},
  {"_open",           open},
  {"_read",           read},
  /* {"_setmode",       setmode}, */
  {"_stricmp",        stricmp},
  {"_strnicmp",       strnicmp},
  {"_strupr",         strupr},
  {"_unlink",         unlink},
  {"_vsnprintf",      vsnprintf},
  {"_write",          write},
  {"abort",           abort},
  {"atexit",          atexit},
  {"bsearch",         bsearch},
  {"calloc",          calloc},
  {"clearerr",        clearerr},
  {"exit",            exit},
  {"fclose",          fclose},
  {"fflush",          fd32_imp$fflush},
  {"fgetc",           fgetc},
  {"fgets",           fgets},
  {"fopen",           fopen},
  {"fprintf",         fd32_imp$fprintf},
  {"fputc",           fd32_imp$fputc},
  {"fputs",           fd32_imp$fputs},
  {"fread",           fd32_imp$fread},
  {"free",            free},
  {"fseek",           fseek},
  {"ftell",           ftell},
  {"fwrite",          fd32_imp$fwrite},
  {"getenv",          getenv},
  {"localtime",       localtime},
  {"malloc",          malloc},
  {"memchr",          memchr},
  {"memcpy",          memcpy},
  {"memmove",         memmove},
  {"memset",          memset},
  {"perror",          perror},
  {"putchar",         putchar},
  {"puts",            puts},
  {"printf",          printf},
  {"qsort",           qsort},
  {"realloc",         realloc},
  {"remove",          remove},
  {"rename",          rename},
  {"setbuf",          setbuf},
  {"setlocale",       setlocale},
  {"signal",          signal},
  {"sprintf",         sprintf},
  {"sscanf",          sscanf},
  {"strcat",          strcat},
  {"strchr",          strchr},
  {"strcpy",          strcpy},
  {"strlen",          strlen},
  {"strncat",         strncat},
  {"strncmp",         strncmp},
  {"strncpy",         strncpy},
  {"strrchr",         strrchr},
  {"strtok",          strtok},
  {"system",          system},
  {"time",            time},
  {"tolower",         tolower},
  {"toupper",         toupper},
  {"vfprintf",        fd32_imp$vfprintf}
};
static uint32_t msvcrt_symnum = sizeof(msvcrt_symarray)/sizeof(struct symbol);

void install_msvcrt(void)
{
  add_dll_table(msvcrt_name, HANDLE_OF_MSVCRT, msvcrt_symnum, msvcrt_symarray);
  add_dll_table(msvcr7_name, HANDLE_OF_MSVCRT+1, msvcrt_symnum, msvcrt_symarray);
}
