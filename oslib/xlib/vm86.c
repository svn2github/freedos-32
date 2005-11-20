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

/* File: Vm86.C
 *	   			
 * VM86 mode switch routines!
 * This is basically an alternative way of invoking the
 * BIOS service routines; it is very useful to support
 * native VBE compliant Video card, without writing an explicit driver
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/mem.h>
#include <ll/i386/x-bios.h>
#include <ll/i386/x-dosmem.h>
#include <ll/i386/cons.h>
#include <ll/i386/error.h>

FILE(VM-86);

/*
#define __LL_DEBUG__
*/

#define VM86_STACK_SIZE 1024

extern DWORD ll_irq_table[256];
static DWORD vm86_stack_size;
/* TSS optional section */
static BYTE  vm86_stack0[VM86_STACK_SIZE];

static struct {
  struct tss t;
  DWORD io_map[2048];
} vm86_tss;
static LIN_ADDR vm86_stack;
static LIN_ADDR vm86_iret_address;


struct registers *global_regs;

static BYTE vm86_return_address[] = {0xcd, 0x48};	/* int 48h */

struct tss *vm86_get_tss(void)
{
  return &(vm86_tss.t);
}

/* This is the return point from V86 mode, called through int 0x48 
 * (see vm86-exc.s). We double check that this function is called in
 * the V86 TSS. Otherwise, Panic!!!
 */
static void vm86_return(DWORD n, struct registers r)
{
  CONTEXT c = get_tr();
#ifdef __LL_DEBUG__
  DWORD cs,eip;
  void *esp;
  DWORD a;
/*    message("Gotta code=%d [0 called from GPF/1 int 0x48]\n",code);*/
#endif
  if (c == X_VM86_TSS) {
    WORD bl;

    global_regs = &r;
#ifdef __LL_DEBUG__
    message("TSS CS=%x IP=%lx\n",vm86_tss.t.cs, vm86_tss.t.eip);
		message("Switching to %x\n", vm86_tss.t.back_link);
    a = (DWORD)(vm86_iret_address);
    cs = (a & 0xFF000) >> 4;
    eip = (a & 0xFFF);
    message("Real-Mode Address is CS=%lx IP=%lx\nLinear=%lx\n",
		cs, eip, a);
    esp = /* (void *)(tos)*/ 0x69;	/* ??? */
    message("Stack frame: %p %lx %lx\n", 
		esp, vm86_tss.t.esp0, vm86_tss.t.esp);
    message("%lx ", lmempeekd(esp));	/* bp */
    message("%lx ", lmempeekd(esp + 4));	/* eip */
    message("%lx ", lmempeekd(esp + 8));	/* 0x0d */
    message("%lx\n", lmempeekd(esp + 12));	/* error code */
/* The error code is given by the selector causing shifted and or-ed with
   3 bits: [LDT/GDT | IDT | Ext/Int]
   If IDT == 1 -> the fault was provoked bu an interrupt (Internal if the
   Ext/Int bit is 0, External if the bit is 1)
   Else the LDT/GDT bit shows if the selector belongs to the LDT (if 1)
   or GDT (if 0)  
*/
    message("%lx ", lmempeekd(esp + 16));	/* EIP of faulting instr */
    message("%lx ", lmempeekd(esp + 20));	/* CS of faulting instr */
    message("%lx ", lmempeekd(esp + 24));	/* EFLAGS */
    message("%lx\n", lmempeekd(esp + 28));	/* old ESP */
    message("%lx ", lmempeekd(esp + 32));	/* old SS */
    message("%lx ", lmempeekd(esp + 36));	/* old ES */
    message("%lx ", lmempeekd(esp + 40));	/* old DS */
    message("%lx\n", lmempeekd(esp + 44));	/* old FS */
#endif
    bl = vm86_tss.t.back_link;
    vm86_tss.t.back_link = 0;
    ll_context_load(bl);
  }
  
  message("INT 0x48 fired outside VM86... Why?\n");
  halt();
}

/* Just a debugging function; it dumps the status of the TSS */
void vm86_dump_tss(void)
{
  BYTE acc,gran;
  DWORD base,lim;

  message("vm86_tss.t dump\n");
  message("Flag: %lx\n", vm86_tss.t.eflags);
  message("SS: %hx SP:%lx\n", vm86_tss.t.ss, vm86_tss.t.esp);
  message("Stack0: %hx:%lx\n", vm86_tss.t.ss0, vm86_tss.t.esp0);
  message("Stack1: %hx:%lx\n", vm86_tss.t.ss1, vm86_tss.t.esp1);
  message("Stack2: %hx:%lx\n", vm86_tss.t.ss2, vm86_tss.t.esp2);
  message("CS: %hx IP: %lx", vm86_tss.t.cs, vm86_tss.t.eip);
  message("DS: %hx\n",vm86_tss.t.ds);
  base = gdt_read(X_VM86_TSS, &lim, &acc, &gran);
  message("Base : %lx Lim : %lx Acc : %x Gran %x\n",
		base,lim,(unsigned)(acc),(unsigned)(gran));
}

void vm86_init(LIN_ADDR dos_mem_buff, DWORD dos_mem_buff_size)
{
  DWORD i;
  
  /* First of all, we need to setup a GDT entries to
   * allow vm86 task execution. We just need a free 386 TSS, which
   * will be used to store the execution context of the virtual 8086
   * task
   */
  gdt_place(X_VM86_TSS,(DWORD)(&vm86_tss),
		sizeof(vm86_tss),FREE_TSS386,GRAN_16);

  l1_int_bind(0x48, vm86_return);

  /* Prepare a real-mode stack, obtaining it from the
   * DOS memory allocator!
   * 8K should be OK! Stack top is vm86_stack + SIZE!
   */
  vm86_stack_size = (dos_mem_buff_size - sizeof(vm86_return_address) - 1) / 2;
  vm86_stack = dos_mem_buff + vm86_stack_size / 2;

  /* Create a location of DOS memory containing the 
   * opcode sequence which will generate a GPF 
   * We use the privileged instruction hlt to do it
   */
  vm86_iret_address = dos_mem_buff + dos_mem_buff_size - sizeof(vm86_return_address);
  memcpy(vm86_iret_address, vm86_return_address, sizeof(vm86_return_address));

#ifdef __LL_DEBUG__
  message("PM reentry linear address=0x%lx\n", (DWORD)vm86_iret_address);
#endif

  /* Zero the PM/Ring[1,2] ss:esp; they're unused! */
  vm86_tss.t.esp1 = 0;
  vm86_tss.t.esp2 = 0;
  vm86_tss.t.ss1 = 0;
  vm86_tss.t.ss2 = 0;
  /* Use only the GDT */
  vm86_tss.t.ldt = 0;
  /* No paging activated */
  vm86_tss.t.cr3 = 0;
  vm86_tss.t.trap = 0;
  /* Yeah, free access to any I/O port; we trust BIOS anyway! */
  /* Here is the explanation: we have 65536 I/O ports... each bit
   * in the io_map masks/unmasks the exception for the given I/O port
   * If the bit is set, an exception is generated; otherwise, if the bit
   * is clear, everythings works fine...
   * Because of alignment problem, we need to add an extra byte all set
   * to 1, according to Intel manuals
   */
  vm86_tss.t.io_base = (DWORD)(&(vm86_tss.io_map)) -
		(DWORD)(&(vm86_tss));
  for (i = 0; i < 2047; i++) vm86_tss.io_map[i] = 0;
  vm86_tss.io_map[2047] = 0xFF000000;
  vm86_tss.t.back_link = 0;
}

int vm86_callBIOS(int service, X_REGS16 *in, X_REGS16 *out, X_SREGS16 *s)
{
  DWORD vm86_tmp_addr;
  DWORD vm86_flags, vm86_cs, vm86_ip;
  LIN_ADDR vm86_stack_ptr;
  DWORD *irq_table_entry;
  
  /* if (service < 0x10 || in == NULL) return -1; */
  if (vm86_tss.t.back_link != 0) {
    message("WTF? Interrupt 0x%x called with CS = 0x%x\n", service, vm86_tss.t.cs);
    halt();
  }
  /* Setup the stack frame */
  vm86_tmp_addr = (DWORD)(vm86_stack);
  vm86_tss.t.ss = (vm86_tmp_addr & 0xFF000) >> 4;
  vm86_tss.t.ebp = (vm86_tmp_addr & 0x0FFF) + vm86_stack_size - 6;
  vm86_tss.t.esp = (vm86_tmp_addr & 0x0FFF) + vm86_stack_size - 6;
  /* Build an iret stack frame which returns to vm86_iretAddress */
  vm86_tmp_addr = (DWORD)(vm86_iret_address);
  vm86_cs = (vm86_tmp_addr & 0xFF000) >> 4;
  vm86_ip = (vm86_tmp_addr & 0xFFF);
  vm86_flags = 0; /* CPU_FLAG_VM | CPU_FLAG_IOPL; */
  vm86_stack_ptr = vm86_stack + vm86_stack_size;
  lmempokew(vm86_stack_ptr - 6, vm86_ip);
  lmempokew(vm86_stack_ptr - 4, vm86_cs);
  lmempokew(vm86_stack_ptr - 2, vm86_flags);
#ifdef __LL_DEBUG__
  message("Stack: %lx SS: %lx SP: %lx\n",
	vm86_tmp_addr + vm86_stack_size,
	(DWORD)vm86_tss.t.ss, vm86_tss.t.esp);
#endif
  /* Wanted VM86 mode + IOPL = 3! */
  vm86_tss.t.eflags = CPU_FLAG_VM | CPU_FLAG_IOPL;
  /* Preload some standard values into the registers */
  vm86_tss.t.ss0 = X_FLATDATA_SEL;
  vm86_tss.t.esp0 = (DWORD)vm86_stack0+VM86_STACK_SIZE; 
  
  /* Copy the parms from the X_*REGS structures in the vm86 TSS */
  vm86_tss.t.eax = (DWORD)in->x.ax;
  vm86_tss.t.ebx = (DWORD)in->x.bx;
  vm86_tss.t.ecx = (DWORD)in->x.cx;
  vm86_tss.t.edx = (DWORD)in->x.dx;
  vm86_tss.t.esi = (DWORD)in->x.si;
  vm86_tss.t.edi = (DWORD)in->x.di;
  /* IF Segment registers are required, copy them... */
  if (s != NULL) {
      vm86_tss.t.es = (WORD)s->es;
      vm86_tss.t.ds = (WORD)s->ds;
  } else {
      vm86_tss.t.ds = vm86_tss.t.ss;
      vm86_tss.t.es = vm86_tss.t.ss; 
  }
  vm86_tss.t.gs = vm86_tss.t.ss; 
  vm86_tss.t.fs = vm86_tss.t.ss; 
  /* Execute the BIOS call, fetching the CS:IP of the real interrupt
   * handler from 0:0 (DOS irq table!)
   */
  irq_table_entry = (void *)(0L);
  vm86_tss.t.cs = ((irq_table_entry[service]) & 0xFFFF0000) >> 16;
  vm86_tss.t.eip = ((irq_table_entry[service]) & 0x0000FFFF);
#ifdef __LL_DEBUG__    
  message("CS:%x IP:%lx\n", vm86_tss.t.cs, vm86_tss.t.eip);
#endif
  /* Let's use the ll standard call... */
  vm86_tss.t.back_link = ll_context_save();
  if (out != NULL) {
    ll_context_load(X_VM86_TSS);
  } else {
    ll_context_to(X_VM86_TSS);
  }

#ifdef __LL_DEBUG__    
  message("I am back...\n");
  message("TSS CS=%hx IP=%lx\n", vm86_tss.t.cs, vm86_tss.t.eip);
  { char *xp = (char *)(vm86_iret_address + 0xe);
    message("PM reentry linear address=%p\n", vm86_iret_address);
    message("Executing code: %x %x\n", xp[0], xp[1]);
  }
#endif
  /* Send back in the X_*REGS structure the value obtained with
   * the real-mode interrupt call
   */
  if (out != NULL) {
    if (vm86_tss.t.back_link != 0) {message("Panic!\n"); halt();}
/*
    out->x.ax = (WORD)vm86_TSS.t.eax;
    out->x.bx = (WORD)vm86_TSS.t.ebx;
    out->x.cx = (WORD)vm86_TSS.t.ecx;
    out->x.dx = (WORD)vm86_TSS.t.edx;
    out->x.si = (WORD)vm86_TSS.t.esi;
    out->x.di = (WORD)vm86_TSS.t.edi;
    out->x.cflag = (WORD)vm86_TSS.t.eflags;
*/
#ifdef __LL_DEBUG__
    message("EAX: 0x%lx\n", global_regs->eax);
    message("EBX: 0x%lx\n", global_regs->ebx);
    message("ECX: 0x%lx\n", global_regs->ecx);
    message("EDX: 0x%lx\n", global_regs->edx);
    message("ESI: 0x%lx\n", global_regs->esi);
    message("EDI: 0x%lx\n", global_regs->edi);
    message("Flags: 0x%x\n", global_regs->flags);
#endif
    out->x.ax = global_regs->eax;
    out->x.bx = global_regs->ebx;
    out->x.cx = global_regs->ecx;
    out->x.dx = global_regs->edx;
    out->x.si = global_regs->esi;
    out->x.di = global_regs->edi;
    out->x.cflag = global_regs->flags;
  }
  if (s != NULL) {
    s->es = vm86_tss.t.es;
    s->ds = vm86_tss.t.ds;
  }
  return 1;
}

/* TODO: How to make vm86_callBIOS and vm86_call co-exist */

#define KERNEL_TEMP_STACK_SIZE 512
/* NOTE: FD32 memory functions */
extern DWORD mem_get(DWORD amount);
extern int mem_free(DWORD base, DWORD size);

typedef struct vm86_call_params {
  DWORD ret_eip;
  DWORD ip;
  DWORD sp;
  X_REGS16 *in_regs;
  X_REGS16 *out_regs;
  X_SREGS16 *seg_regs;
  struct tss *ps_tss_ptr;
  struct vm86_call_params *params_handle;
} vm86_call_params_t;

int vm86_call(WORD ip, WORD sp, X_REGS16 *in, X_REGS16 *out, X_SREGS16 *s, struct tss *ps_tss, void *params_handle)
{
  int res;
  WORD bl = vm86_tss.t.back_link;

  if (params_handle != NULL) {
    /* Save the previous vm86 tss, ret_eip not used
       ((vm86_call_params_t *)params_handle)->ret_eip = vm86_tss.t.eip; 
     */
    memcpy(((vm86_call_params_t *)params_handle)->ps_tss_ptr, &vm86_tss.t, sizeof(struct tss));
  }

  if (bl != 0) { /* Nested vm86 process */
    struct tss prev_kernel_tss;
    DWORD kernel_stack;
    vm86_call_params_t *p_vm86_call_params;
    struct tss * kernel_tss_ptr = (struct tss *)gdt_read(bl, NULL, NULL, NULL);

    /* Go back to the kernel process and run vm86_call again ... */
    vm86_tss.t.back_link = 0;
    memcpy(&prev_kernel_tss, kernel_tss_ptr, sizeof(struct tss));
    /* Load another kernel mode process */
    kernel_tss_ptr->eip = (DWORD)vm86_call;
    kernel_tss_ptr->ss  = X_FLATDATA_SEL;
    /* Create a new kernel stack */
    kernel_stack = mem_get(KERNEL_TEMP_STACK_SIZE);
    kernel_tss_ptr->esp = kernel_stack+KERNEL_TEMP_STACK_SIZE-sizeof(vm86_call_params_t);
    p_vm86_call_params = (vm86_call_params_t *)kernel_tss_ptr->esp;
    p_vm86_call_params->ip = ip;
    p_vm86_call_params->sp = sp;
    p_vm86_call_params->in_regs = in;
    p_vm86_call_params->out_regs = out;
    p_vm86_call_params->seg_regs = s;
    p_vm86_call_params->ps_tss_ptr = ps_tss;
    p_vm86_call_params->params_handle = p_vm86_call_params;
    ll_context_load(bl);
    mem_free(kernel_stack, KERNEL_TEMP_STACK_SIZE);
    memcpy(kernel_tss_ptr, &prev_kernel_tss, sizeof(struct tss));
#ifdef __LL_DEBUG__
    message("Nested vm86 call returned!\n");
#endif
    res = vm86_tss.t.eax;
/*  The vm86 application in vm86 mode is done
    message("WTF? VM86 called with CS = 0x%x, IP = 0x%x\n", vm86_tss.t.cs, ip);
    fd32_abort();
*/
  } else {
    /* Setup the stack frame */
    vm86_tss.t.ss = s->ss;
    vm86_tss.t.ebp = 0x91E; /* this is more or less random but some programs expect 0x9 in the high byte of BP!! */
    vm86_tss.t.esp = sp;
    /* Wanted VM86 mode + IOPL = 3! */
    vm86_tss.t.eflags = CPU_FLAG_VM | CPU_FLAG_IOPL | CPU_FLAG_IF;
    /* Ring0 system stack */
    vm86_tss.t.ss0 = X_FLATDATA_SEL;
    vm86_tss.t.esp0 = (DWORD)vm86_stack0+VM86_STACK_SIZE;
    /* Copy the parms from the X_*REGS structures in the vm86 TSS */
    vm86_tss.t.eax = in->x.ax;
    vm86_tss.t.ebx = in->x.bx;
    vm86_tss.t.ecx = in->x.cx;
    vm86_tss.t.edx = in->x.dx;
    vm86_tss.t.esi = in->x.si;
    vm86_tss.t.edi = in->x.di;
    /* IF Segment registers are required, copy them... */
    vm86_tss.t.es = s->es;
    vm86_tss.t.ds = s->ds;
    vm86_tss.t.gs = s->ss;
    vm86_tss.t.fs = s->ss;
    /* Execute the CS:IP */
    vm86_tss.t.cs = s->cs;
    vm86_tss.t.eip = ip;
    /* Let's use the ll standard call... */
    vm86_tss.t.back_link = ll_context_save();

    if (out != NULL) {
      ll_context_load(X_VM86_TSS);
    } else {
      ll_context_to(X_VM86_TSS);
    }

    /* Send back in the X_*REGS structure the value obtained with
     * the real-mode interrupt call
     */
    if (out != NULL) {
      if (vm86_tss.t.back_link != 0) {message("Panic!\n"); halt();}
  
      out->x.ax = vm86_tss.t.eax;
      out->x.bx = vm86_tss.t.ebx;
      out->x.cx = vm86_tss.t.ecx;
      out->x.dx = vm86_tss.t.edx;
      out->x.si = vm86_tss.t.esi;
      out->x.di = vm86_tss.t.edi;
      out->x.cflag = vm86_tss.t.eflags;
    }
    if (s != NULL) {
      s->es = vm86_tss.t.es;
      s->ds = vm86_tss.t.ds;
    }

    res = vm86_tss.t.eax;
  }

  /* Load and back to the previous vm86 application */
  if (params_handle != NULL) {
    memcpy(&vm86_tss.t, ((vm86_call_params_t *)params_handle)->ps_tss_ptr, sizeof(struct tss));
    vm86_tss.t.back_link = ll_context_save();
    vm86_tss.t.eax = res;
    ll_context_load(X_VM86_TSS);
  }

  return res;
}
