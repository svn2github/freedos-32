/* Mini MSVCRT
 * by Hanzac Chen
 *
 * Ref: WINE's msvcrt.dll
 *
 * This is free software; see GPL.txt
 */


#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

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

static char ** fd32_environ;
static char *** fd32_imp__p__environ(void)
{
  return &fd32_environ;
}

static char ** env;
static char *** p___ddddenv(void)
{
  return &env;
}

static unsigned int* p__fmode(void)
{
  return NULL;
}

uint32_t mem_limit;
extern struct psp *current_psp;
static void fd32_imp__getmainargs(int *argc, char** *argv, char** *envp, int expand_wildcards, int *new_mode)
{
  fd32_log_printf("[WINB] GETMAINARGS: (also set the memlimit (0x%x) here)\n", current_psp->memlimit);
  fd32_log_printf("[WINB] Current PSP cmdline: %s (%d)\n", current_psp->command_line, current_psp->command_line_len);
  mem_limit = current_psp->memlimit;
}

static void cexit(void)
{
}

void restore_sp(uint32_t s);

static uint32_t iob[3];

static uint32_t onexit(void * func)
{
  puts("onexit");
  return 0;
}

static int fd32_imp_setmode(int fd, int mode)
{
  return 0;
}

static void amsg_exit(int errnum)
{
  printf("errnum: (%d)\n", errnum);
}

static int XcptFilter(int ex, uint32_t ptr)
{
  puts("XcptFilter");
  return 0;
}

typedef void (*_INITTERMFUN)(void);
static unsigned int initterm(_INITTERMFUN *start,_INITTERMFUN *end)
{
  _INITTERMFUN* current = start;

  printf("(%p,%p)\n",start,end);
  while (current<end)
  {
    if (*current)
    {
      printf("Call init function %p\n", *current);
      (**current)();
      printf("returned\n");
    }
    current++;
  }
  return 0;
}

static void adjust_fdiv(void)
{

}

static unsigned int commode;
static unsigned int* p__commode(void)
{
  return &commode;
}

static unsigned int controlfp(unsigned int newval, unsigned int mask)
{
  puts("controlfp:Not Implemented!");
  return 0;
}

static int except_handler3(uint32_t rec, void* frame, uint32_t context, void* dispatcher)
{
  puts("except_handler3:Not Implemented!");
  return 0;
}

static void setusermatherr(void * func)
{
  printf(":new matherr handler %p\n", func);
}

/* from Newlib */
extern int *__errno(void);

static char msvcrt_name[] = "msvcrt.dll";
static char msvcr7_name[] = "msvcr70.dll";
static uint32_t msvcrt_handle = 0x05;
static struct symbol msvcrt_symarray[] = {
  {"__dllonexit",    (uint32_t)fd32_imp__dllonexit},
  {"__getmainargs",  (uint32_t)fd32_imp__getmainargs},
  {"__p___initenv",  (uint32_t)p___ddddenv},
  {"__p__commode",   (uint32_t)p__commode},
  {"__p__environ",   (uint32_t)fd32_imp__p__environ},
  {"__p__fmode",     (uint32_t)p__fmode},
  {"__set_app_type", (uint32_t)fd32_imp__set_app_type},
  {"__setusermatherr", (uint32_t)setusermatherr},
  {"_adjust_fdiv",   (uint32_t)adjust_fdiv},
  {"_amsg_exit",     (uint32_t)amsg_exit},
  {"_controlfp",     (uint32_t)controlfp},
  {"_cexit",         (uint32_t)cexit},
  {"_except_handler3", (uint32_t)except_handler3},

  {"_initterm",      (uint32_t)initterm},
  {"_iob",           (uint32_t)iob},
  {"_onexit",        (uint32_t)onexit},
  {"_setmode",       (uint32_t)fd32_imp_setmode},
  {"_XcptFilter",    (uint32_t)XcptFilter},

  /* from Newlib */
  {"_errno",         (uint32_t)__errno},
  {"_exit",          (uint32_t)exit},
  {"_fdopen",        (uint32_t)fdopen},
  {"_unlink",        (uint32_t)unlink},
  {"_vsnprintf",     (uint32_t)vsnprintf},
  {"abort",          (uint32_t)abort},
  {"atexit",         (uint32_t)atexit},
  {"clearerr",       (uint32_t)clearerr},
  {"exit",           (uint32_t)exit},
  {"fclose",         (uint32_t)fclose},
  {"fflush",         (uint32_t)fflush},
  {"fopen",          (uint32_t)fopen},
  {"fprintf",        (uint32_t)fprintf},
  {"fputc",          (uint32_t)fputc},
  {"fread",          (uint32_t)fread},
  {"free",           (uint32_t)free},
  {"fseek",          (uint32_t)fseek},
  {"ftell",          (uint32_t)ftell},
  {"fwrite",         (uint32_t)fwrite},
  {"malloc",         (uint32_t)malloc},
  {"memcpy",         (uint32_t)memcpy},
  {"memset",         (uint32_t)memset},
  {"perror",         (uint32_t)perror},
  {"puts",           (uint32_t)puts},
  {"printf",         (uint32_t)printf},
  {"signal",         (uint32_t)signal},
  {"sprintf",        (uint32_t)sprintf},
  {"strcat",         (uint32_t)strcat},
  {"strcpy",         (uint32_t)strcpy},
  {"strlen",         (uint32_t)strlen},
  {"system",         (uint32_t)system}
};
static uint32_t msvcrt_symnum = sizeof(msvcrt_symarray)/sizeof(struct symbol);

void install_msvcrt(void)
{
  add_dll_table(msvcrt_name, msvcrt_handle, msvcrt_symnum, msvcrt_symarray);
  add_dll_table(msvcr7_name, msvcrt_handle+1, msvcrt_symnum, msvcrt_symarray);
}
