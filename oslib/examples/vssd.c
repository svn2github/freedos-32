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

/*	Very Simple Scheduling Demo (VSSD)	*/

#include <ll/i386/cons.h>
#include <ll/i386/tss-ctx.h>
#include <ll/i386/error.h>
#include <ll/sys/ll/ll-func.h>
#include <ll/sys/ll/time.h>
#include <ll/sys/ll/event.h>
#include <ll/stdlib.h>

#define T 1000

#define S0 50000
#define S1 50000
#define S2 50000
struct thread {
    CONTEXT ctx;
    struct thread *next;
    DWORD slice;
};

#define STACK_SIZE      100000U	/* Stack size in bytes          */
#define USE_FPU         0x0001

#define TO     ll_context_to
#define FROM   ll_context_from

/* For stack allocation checking */
BYTE stacks[STACK_SIZE];
struct thread *exec;

void scheduler(void *p)
{
    struct timespec time;

    ll_gettime(TIME_NEW, &time);
    ADDNANO2TIMESPEC(exec->slice * 1000, &time);
    event_post(time, scheduler, NULL);
    exec = exec->next;
    TO(exec->ctx);
}

void killer(void)
{
    struct thread *nth;

    cli();
    message("Killing a thread...\n");
    nth = exec->next;
    if (nth == exec) {
        error("Killer: New Thread == Killed Thread... This cannot happen!!!\n");
	ll_abort(100);
    }
    exec = nth;
    sti();
    TO(exec->ctx);
}

void th1_body(void *p)
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

void th2_body(void *p)
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
    DWORD sp1, sp2;
    struct ll_initparms parms;
    void *mbi;
    DWORD t, oldt, secs;
    struct timespec sched_time;
    struct thread main_thread;
    struct thread thread1;
    struct thread thread2;

    sp1 = get_sp();
    cli();
    parms.mode = LL_ONESHOT;
    parms.tick = T;

    mbi = ll_init();
    event_init(&parms);

    if (mbi == NULL) {
	message("Error in LowLevel initialization code...\n");
	sti();
	l1_exit(-1);
    }
    message("LowLevel started...\n");
    main_thread.ctx = ll_context_save();
    main_thread.slice = S0;
    main_thread.next = &thread1;
    thread1.ctx = ll_context_create(th1_body, &stacks[STACK_SIZE / 2],
			      NULL, killer, 0);
    thread1.slice = S1;
    thread1.next = &thread2;
    thread2.ctx = ll_context_create(th2_body, &stacks[STACK_SIZE],
			      NULL, killer, 0);
    thread2.slice = S2;
    thread2.next = &main_thread;
    if (thread1.ctx == 0) {
        error("Unable to create thread 1");
	    ll_abort(100);
    }
    if (thread2.ctx == 0) {
        error("Unable to create thread 2");
	    ll_abort(100);
    }
    
    message("Threads created...\n");
    exec = &main_thread;
    ll_gettime(TIME_NEW, &sched_time);
    ADDNANO2TIMESPEC(5000000, &sched_time);

    event_post(sched_time, scheduler, NULL);
    sti();

    secs = 0;
    oldt = 0;
    while (secs <= 20) {
	cli();
	t = ll_gettime(TIME_NEW, NULL);
	sti();
	if (t < oldt) {
	    message("ARGGGGG! %lu %lu\n", t, oldt);
	    ll_abort(99);
	}
	oldt = t;
	if ((t / 1000000) > secs) {
	    secs++;
	    printf_xy(40, 20, WHITE, "%lu     %lu    ", secs, t);
	}
    }
    message("\n DONE: Secs = %lu\n", secs);

    cli();
    ll_end();
    sp2 = get_sp();
    message("End reached!\n");
    message("Actual stack : %lx - ", sp2);
    message("Begin stack : %lx\n", sp1);
    message("Check if same : %s\n", sp1 == sp2 ? "Ok :-)" : "No :-(");
    return 1;
}
