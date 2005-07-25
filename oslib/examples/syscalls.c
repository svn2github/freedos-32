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

/*	Address Spaces test file			*/

#include <ll/i386/stdlib.h>
#include <ll/i386/error.h>
#include <ll/i386/mem.h>
#include <ll/i386/hw-arch.h>
#include <ll/i386/farptr.h>
#include <ll/sys/ll/ll-func.h>
#include <ll/sys/ll/aspace.h>
#include <ll/sys/ll/event.h>
#include <ll/string.h>

#define USE_NEW_HANDLERS
#define T  1000
#define WAIT()  for (w = 0; w < 0x5FFFFF; w++)                                 

extern DWORD GDT_base;
extern void int0x31(void);

#ifndef USE_NEW_HANDLERS
struct regs {
  DWORD flags;
  DWORD egs;
  DWORD efs;
  DWORD ees;
  DWORD eds;
  DWORD edi;
  DWORD esi;
  DWORD ebp;
  DWORD esp;
  DWORD ebx;
  DWORD edx;
  DWORD ecx;
  DWORD eax;
};
#endif

BYTE space[2048];
WORD th1, th2, thm;

char outstring[] = "Hi there!!!\n";

#ifdef USE_NEW_HANDLERS
void chandler(DWORD intnum, struct registers r)
#else
void chandler(struct regs r, DWORD intnum)
#endif
{
  message("[System Call] EAX = 0x%lx EBX = 0x%lx ECX = 0x%lx...\n",
		  r.eax, r.ebx, r.ecx);

  if (r.eax == 1) {
    message("Exit!!!\n");
    ll_context_load(thm);
  }

  if (r.eax == 2) {
    char string[20];
    DWORD p;
    unsigned int size;

    p = gdt_read(r.eds, NULL, NULL, NULL);
    p += r.ebx;
    size = r.ecx;
    if (size > 20) {
      size = 20;
    }
#if 0
    message("Buffer @0x%lx, len %u\n", p, size);
    l1_end();
    sti();
    l1_exit(0);
#endif

    memcpy(string, (void *)p, size);
    string[19] = 0;
    message("Write...\n");
    message("%s", string);
    return;
  }

  message("Unsupported System Call!!!\n");
}


/* For now, this is not called */
void killer(void)
{
  cli();
  message("Killer!!!\n");
  ll_context_load(thm);
}

/*
  And this is the thread that runs in the new AS... It cannot call
  any function (it is alone in its AS), so it simply plots a star
  on the screen and loops waiting for an event that switches to the
  main thread.
*/
void nullfun(void *p)
{
  /* Some asm to write on the screen (we cannot use libc... it is not
     linked in this AS */
  __asm__ __volatile__ ("movl $2, %eax");
  __asm__ __volatile__ ("movl $1000, %ebx");
  __asm__ __volatile__ ("movl $16, %ecx");
  __asm__ __volatile__ ("int $0x31");
  __asm__ __volatile__ ("movl $1, %eax");
  __asm__ __volatile__ ("int $0x31");

  /* should not arrive here... */
  for(;;);
  halt();
}

int main (int argc, char *argv[])
{
  DWORD sp1, sp2;
  void *res;
  AS as, ds;
  int ok;
  
  sp1 = get_sp();
  cli();
  res = ll_init();

  if (res == NULL) {
    message("Error in LowLevel initialization code...\n");
    sti();
    l1_exit(-1);
  }
  message("LowLevel started...\n");
  
#ifdef USE_NEW_HANDLERS
  l1_int_bind(0x31, chandler);
#else
  idt_place(0x31, int0x31);
#endif
  as_init();

  message("Creating an Address Space\n");
  as = as_create();
  message("Created: ID = %d %x\n", as, as);
  message("Binding array @ 0x%lx\n", (DWORD)space);
  ok = as_bind(as, (DWORD)space, 0, sizeof(space));

  if (ok < 0) {
    message("Error in as_bind!!!\n");
  } else {
    message("Bound\n");
  }

  ds = get_ds();
  /* Let's create the image of the process...
   * 0 --> 1000 : text
   * 1000 --> 1000 + strlen(outstring) : initialized data
   * ? <-- 2048 : stack
   */
  fmemcpy(as, 0, ds, (DWORD)nullfun, 1000);
  fmemcpy(as, 1000, ds, (DWORD)outstring, strlen(outstring));
  /* Create a thread (in our AS) and a task (in a different AS) */
  th1 = ll_context_create(0, (void *)2048, NULL,killer, 0);
  ll_context_setspace(th1, as);
    	
  /* Save our context, in order to permit to switch back to main */
  thm = ll_context_save();
  message("I am thread %x\n", thm);
  message("Thread 1 created\n");
  message("Switching to it...\n");
  ll_context_load(th1);
  message("Back to Main\n");

  cli();
  ll_end();
  sp2 = get_sp();
  message("End reached!\n");
  message("Actual stack : %lx - ", sp2);
  message("Begin stack : %lx\n", sp1);
  message("Check if same : %s\n",sp1 == sp2 ? "Ok :-)" : "No :-(");
  
  return 1;
}
