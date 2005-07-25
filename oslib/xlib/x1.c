/* Project:     OSLib
 * Description: The OS Construction Kit
 * Date:                1.6.2000
 * Idea by:             Luca Abeni & Gerardo Lamastra
 *
 * OSLib is an SO project aimed at developing a common, easy-to-use
 * low-level infrastructure for developing OS kernels and Embedded
 * Applications; it partially derives from the HARTIK project but it
 * currently is independently developed.
 *
 * OSLib is distributed under GPL License, and some of its code has
 * been derived from the Linux kernel source; also some important
 * ideas come from studying the DJGPP go32 extender.
 *
 * We acknowledge the Linux Community, Free Software Foundation,
 * D.J. Delorie and all the other developers who believe in the
 * freedom of software and ideas.
 *
 * For legalese, check out the included GPL license.
 */

/* File: X1.C       */
/* Startup code:        */
/* Build parameters list & make info accessible */

#include <ll/i386/hw-func.h>
#include <ll/stdlib.h>
#include <ll/i386/cons.h>
#include <ll/i386/mb-info.h>

#include <ll/i386/mem.h>

FILE(X1);

/* #define __DUMP_MEM__  */

/* We need to copy from X address space to the application space  */
/* the info structure to allow pointer access using flat model    */
/* Remember that flat model is similar to small model also if we  */
/* can see the whole memory, because it has no explicit far       */
/* pointers; then if we pass into _args[] the address of the      */
/* string of the n-th argument it could not be correctly accessed */
/* because it lies in a memory zone unseen from PM application.   */
/* This is due to the ELF format which has no relocation info     */
/* since the file is already relocated starting from address 0!   */
/* Then the PM application cannot see a real flat memory (segment */
/* with 0 base) but CS,DS & SS MUST have the base correctly set.  */
/* Refer to this figure:                      */
/*                                */
/* DOS Memory <- X is there                   */
/*                                        */
/* EXTENDED Memory -----[                     */
/*          [                     */
/*  Address  xxxx   [ <- Application code is here!        */
/*          [                     */
/*  Address  yyyy   [ <- Application Data & Stack!        */
/*                                */
/* Then CS has xxxx base while DS & SS have yyyy base!        */

/* Stack base address; use this to check stack overflow!    */
/* With Flat model I do not think we can use 386 protection     */
/* to detect a stack overflow; anyway Watcom C use a standard   */
/* function __CHK to detect it! The compiler place it whenever  */
/* it calls a function to detect overflow           */

DWORD _stkbase; 
DWORD _stktop;

/* This is some extra stuff we need to compile with argument    */
/* passing and math extensions                  */
DWORD _argc = 0;
typedef char *charp;
charp _argv[100];

#ifndef MAIN
#define MAIN main
#endif
#define MAX_ARG_SIZE 1024
static char private_args[MAX_ARG_SIZE];

extern void MAIN(int argc,char *argv[]);
extern void bios_save(void);
extern void bios_restore(void);

/* This is used in GNU-C to implement C++ constructors/destructors */
/* See the lib sources for more details                */
void __main(int argc, char **argv)
{
}


struct multiboot_info * mbi_address(void)
{
  /* This is declared in [wc32/gnu]\x0.[asm/s] */
  extern struct multiboot_info *mbi;

  return (mbi);
}

void _startup(void)
{
  register int i = 0;
  struct multiboot_info *mbi = mbi_address();

  if (!(mbi->flags & MB_INFO_MEMORY)) {
    cputs("X/Runtime library error!!! Unable to find memory information!\n");
    l1_exit(-1);
  }
	
  if (mbi->flags & MB_INFO_CMDLINE) {
    /* Build parameter list, up to 100 parms... */
    memcpy(private_args, (char *)mbi->cmdline, MAX_ARG_SIZE);
    private_args[MAX_ARG_SIZE - 1] = 0;
    while (private_args[i] != 0) {
      _argv[_argc] = &(private_args[i]);
      while (private_args[i] != ' ' && private_args[i] != 0) i++;
      if (private_args[i] == ' ') {
	private_args[i] = 0; i++; _argc++;
      }
    }
    _argc++;
  }
  
  bios_save();
  /* Call main procedure using standard C convention   */
  /* Remember you cannot call any console I/O function */
  /* if you do not call bios_save()            */
	
#ifdef __DUMP_MEM__
  message("X/MEM   : %u\n",mbi->mem_upper);
  message("DOS/MEM : %u\n",mbi->mem_lower);
  message("x_bios Size : %u\n",sizeof(X_BIOSCALL));
  message("mbi Size : %u\n",sizeof(struct multiboot_info));
  message("Cmdline : %s\n",mbi->cmdline);
  message("Argc : %u",_argc);
  message("Argv[0] : %s / %s\n",_argv[0]);
  message("Argv[1] : %s\n",_argv[1]);
  message("Argv[2] : %s\n",_argv[2]);
  message("Argv[3] : %s\n",_argv[3]);
#endif
  MAIN(_argc,_argv);
  bios_restore();
}
