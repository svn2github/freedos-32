/* Mini MSVCRT
 * by Hanzac Chen
 *
 * This is free software; see GPL.txt
 */

/* libfd32 and newlib */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>

#include "winb.h"


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

static int* fd32_imp___errno(void)
{
  puts("_errno: not implemented!");
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

static int setmode(int fd,int mode)
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
      printf("Call init function %p\n",*current);
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

/* libfd32 and newlib */
extern void _exit(int res);

static char msvcrt_name[] = "msvcrt.dll";
static char msvcr7_name[] = "msvcr70.dll";
static uint32_t msvcrt_handle = 0x05;
static uint32_t msvcrt_symnum = 0x20;
static struct symbol msvcrt_symarray[0x20] = {
  {"__set_app_type", (uint32_t)fd32_imp__set_app_type},
  {"malloc",         (uint32_t)malloc},
  {"free",           (uint32_t)free},
  {"puts",           (uint32_t)puts},
  {"printf",         (uint32_t)printf},
  {"__p__environ",   (uint32_t)fd32_imp__p__environ},
  {"__p___initenv",  (uint32_t)p___ddddenv},
  {"__getmainargs",  (uint32_t)fd32_imp__getmainargs},
  {"__p__fmode",     (uint32_t)p__fmode},
  {"exit",           (uint32_t)_exit},
  {"_exit",          (uint32_t)_exit},
  {"_cexit",         (uint32_t)cexit},
  {"_c_exit",        (uint32_t)cexit},
  {"_iob",           (uint32_t)iob},
  {"_onexit",        (uint32_t)onexit},
  {"__dllonexit",    (uint32_t)onexit},
  {"_amsg_exit",     (uint32_t)amsg_exit},
  {"_setmode",       (uint32_t)setmode},
  {"abort",          (uint32_t)abort},
  {"_errno",         (uint32_t)fd32_imp___errno},
  {"fflush",         (uint32_t)fflush},
  {"atexit",         (uint32_t)atexit},
  {"signal",         (uint32_t)signal},
  {"_XcptFilter",    (uint32_t)XcptFilter},
  {"_initterm",      (uint32_t)initterm},
  {"_adjust_fdiv",   (uint32_t)adjust_fdiv},
  {"__p__commode",   (uint32_t)p__commode},
  {"_controlfp",     (uint32_t)controlfp},
  {"_except_handler3", (uint32_t)except_handler3},
  {"__setusermatherr", (uint32_t)setusermatherr},
  {"system",         (uint32_t)system}
};


void add_msvcrt(void)
{
  add_dll_table(msvcrt_name, msvcrt_handle, msvcrt_symnum, msvcrt_symarray);
  add_dll_table(msvcr7_name, msvcrt_handle+1, msvcrt_symnum, msvcrt_symarray);
}
