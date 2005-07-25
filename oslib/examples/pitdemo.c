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
/* Based on a test program provided by Juras Benesh (ybxsoft@tut.by) */

#include <ll/stdlib.h>
#include <ll/sys/ll/ll-func.h>
#include <ll/i386/pit.h>
#include <ll/i386/pic.h>
#include <ll/i386/cons.h>

#define MAX_ACTIVATIONS 1000

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


int main(int argc, char *argv[])
{
  char c[] = "Main: /";
  
  cli();
  clear();
  ll_init();

  /* Set periodic mode */
  pit_init(0, TMR_MD2, 0xFFFF);
  l1_irq_bind(0, pit_handler);
  irq_unmask(0);
  sti();

  printf_xy(37, 1, 14, "Hello!");
  
  while(1) {
    if (c[6] == '/') {
      c[6] = '\\';
    } else {
      c[6] = '/';
    }
    printf_xy(60, 2, 15, c);
  }
  
  /* Never arrive here... */
  return 0;
}
