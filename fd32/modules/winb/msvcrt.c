/* Mini MSVCRT
 * by Hanzac Chen
 * 
 * This is free software; see GPL.txt
 */

/* libfd32 and newlib */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>

typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;

#include "format.h"
#include "kernel.h"

void ASSERT(int a)
{
}

static void set_app_type(int a)
{
	printf("The App Type is %d\n", a);
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

extern struct psp *current_psp;
static void getmainargs(int *argc, char** *argv, char** *envp, int expand_wildcards, int *new_mode)
{
/*
	message("getmainargs\n");
	message("current psp arg name: %s %d\n", current_psp->command_line, current_psp->command_line_len);
 */	
}

static void cexit(void)
{
}

void restore_sp(DWORD s);

static DWORD iob[3];

static DWORD onexit(void * func)
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

static int XcptFilter(int ex, DWORD ptr)
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

static int except_handler3(DWORD rec, void* frame, DWORD context, void* dispatcher)
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
static DWORD msvcrt_handle = 0x05;
static DWORD msvcrt_symnum = 0x20;
static struct symbol msvcrt_symarray[0x20] = {
{"__set_app_type", (DWORD)set_app_type},
{"malloc", (DWORD)malloc},
{"free", (DWORD)free},
{"puts", (DWORD)puts},
{"printf", (DWORD)printf},
{"__p__environ", (DWORD)fd32_imp__p__environ},
{"__p___initenv", (DWORD)p___ddddenv},
{"__getmainargs", (DWORD)getmainargs},
{"__p__fmode", (DWORD)p__fmode},
{"exit", (DWORD)_exit},
{"_exit", (DWORD)_exit},
{"_cexit", (DWORD)cexit},
{"_c_exit", (DWORD)cexit},
{"_iob", (DWORD)iob},
{"_onexit", (DWORD)onexit},
{"__dllonexit", (DWORD)onexit},
{"_amsg_exit", (DWORD)amsg_exit},
{"_setmode", (DWORD)setmode},
{"abort", (DWORD)abort},
{"_errno", (DWORD)fd32_imp___errno},
{"fflush", (DWORD)fflush},
{"atexit", (DWORD)atexit},
{"signal", (DWORD)signal},
{"_XcptFilter", (DWORD)XcptFilter},
{"_initterm", (DWORD)initterm},
{"_adjust_fdiv", (DWORD)adjust_fdiv},
{"__p__commode", (DWORD)p__commode},
{"_controlfp", (DWORD)controlfp},
{"_except_handler3", (DWORD)except_handler3},
{"__setusermatherr", (DWORD)setusermatherr},
{"system", (DWORD)system}};


void add_msvcrt(void)
{
	add_dll_table(msvcrt_name, msvcrt_handle, msvcrt_symnum, msvcrt_symarray);
	add_dll_table(msvcr7_name, msvcrt_handle+1, msvcrt_symnum, msvcrt_symarray);
}
