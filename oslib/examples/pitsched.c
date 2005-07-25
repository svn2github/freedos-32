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

/*	Simple Demo showing how to explicitly program the PIT	*/

#include <ll/stdlib.h>
#include <ll/sys/ll/ll-func.h>
#include <ll/i386/pit.h>
#include <ll/i386/pic.h>
#include <ll/i386/cons.h>
#include <ll/i386/error.h>
#include <ll/sys/ll/event.h>

#define MAX_ACTIVATIONS 1000
CONTEXT contexts[3];

char stk0[4096];
char stk1[4096];

#if 1
void pit_handler(void *p)
{
  static int exec = 0;

  if (++exec == 3) {
    exec = 0;
  }
  ll_context_to(contexts[exec]);
}
#else
void pit_handler(void)
{
  static int activations = 0;
  static char c[] = "Timer: |";

  if (activations++ == MAX_ACTIVATIONS) {
    printf_xy(36, 3, 14, "GoodBye!");
    ll_end();
    l1_exit(0);
  }
  if (c[7] == '|') {
    c[7] = '-';
  } else {
    c[7] = '|';
  }
  
  printf_xy(10, 2, 15, c);
}
#endif

void killer(void)
{
    cli();
    ll_end();
    l1_exit(0);
}

void procth0(void *p)
{
  static char c[] = "Thread 1: |";

 
  while(1) {
    if (c[10] == '|') {
      c[10] = '-';
    } else {
      c[10] = '|';
    }
    printf_xy(60, 5, 15, c);
  }
}

void procth1(void *p)
{
  static char c[] = "Thread 2: |";

  while(1) {
    if (c[10] == '|') {
      c[10] = '-';
    } else {
      c[10] = '|';
    }
    printf_xy(20, 5, 15, c);
  }
}

int main(int argc, char *argv[])
{
  char c[] = "Main: /";
  extern unsigned short int currCtx;
  
  cli();
  clear();
  ll_init();

  contexts[0] = currCtx = context_save();
  contexts[1] = ll_context_create(procth0, &stk0[4095], NULL, killer, 0);
  contexts[2] = ll_context_create(procth1, &stk1[4095], NULL, killer, 0);

  message("Contexts: 0x%x 0x%x 0x%x\n", contexts[0], contexts[1], contexts[2]);
  /* Set periodic mode */
  pit_init(0, TMR_MD2, 0xFFFF);
#ifdef __USE_KL__
  irq_bind(0, pit_handler, 0);
#else
  l1_irq_bind(0, pit_handler);
#endif
  irq_unmask(0);
  sti();

  printf_xy(37, 1, 14, "Hello!");
  
  while(1) {
    if (c[6] == '/') {
      c[6] = '\\';
    } else {
      c[6] = '/';
    }
    printf_xy(40, 2, 15, c);
  }
  
  /* Never arrive here... */
  return 0;
}
