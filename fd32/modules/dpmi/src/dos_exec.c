/* DOS16/32 execution for FD32
 * by Luca Abeni & Hanzac Chen
 *
 * This is free software; see GPL.txt
 */

#include <ll/i386/hw-data.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/error.h>
#include <ll/i386/x-bios.h>
#include <ll/string.h>
#include <devices.h>
#include <filesys.h>
#include <format.h>
#include <fcntl.h>
#include <kmem.h>
#include <exec.h>
#include <kernel.h>
#include <logger.h>
#include "dpmi.h"
#include "dos_exec.h"
#include "ldtmanag.h"

#define __DOS_EXEC_DEBUG__

#define ENV_SIZE 0x100

/* Sets the JFT for the current process. */
static void local_set_psp_jft(struct psp *ppsp, void *Jft, int JftSize)
{
  struct process_info *cur_P = fd32_get_current_pi();

  if (Jft == NULL)
    Jft = fd32_init_jft(MAX_OPEN_FILES);
  ppsp->ps_maxfiles = JftSize;
  ppsp->ps_filetab  = Jft;
  cur_P->jft_size   = JftSize;
  cur_P->jft        = Jft;
}

static void local_set_psp_commandline(struct psp *ppsp, char *args)
{
  if (args != NULL) {
    ppsp->command_line_len = strlen(args);
    memcpy(ppsp->command_line, args, ppsp->command_line_len);
    ppsp->command_line[ppsp->command_line_len] = '\r';
  } else {
    ppsp->command_line_len = 0;
    ppsp->command_line[0] = '\r';
  }
}

/* TODO: Re-consider the fcb1 and fcb2 to support multi-tasking */
static DWORD g_fcb1 = 0, g_fcb2 = 0, g_env_segment, g_env_segtmp = 0;
static void local_set_psp(struct psp *ppsp, WORD ps_size, WORD ps_parent, WORD env_sel, WORD stubinfo_sel, DWORD fcb_1, DWORD fcb_2, char *filename, char *args)
{
  BYTE *env_data;
  /* Set the PSP */
  fd32_get_current_pi()->psp = ppsp;
  /* Init PSP */
  ppsp->ps_size = ps_size; /* segment of first byte beyond memory allocated to program  */
  ppsp->ps_parent = ps_parent;
  if (fcb_1 != NULL) {
    fcb_1 = (fcb_1>>12)+(fcb_1&0x0000FFFF);
    memcpy(ppsp->def_fcb_1, (void *)fcb_1, 16);
  }
  if (fcb_2 != NULL) {
    fcb_2 = (fcb_2>>12)+(fcb_2&0x0000FFFF);
    memcpy(ppsp->def_fcb_2, (void *)fcb_2, 20);
  }

  ppsp->ps_environ = env_sel;
  ppsp->stubinfo_sel = stubinfo_sel;
  ppsp->dta = &ppsp->command_line_len;
  /* And now... Set the arg list!!! */
  local_set_psp_commandline(ppsp, args);
#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("[DOSEXEC] New PSP: 0x%x\n", (int)ppsp);
#endif
  /* Create the Job File Table */
  /* TODO: Why!?!? Help me Luca!
           For the moment I always create a new JFT */
  /* if (npsp->link == NULL) Create new JFT
     else Copy JFT from npsp->link->Jft
  */
  local_set_psp_jft(ppsp, NULL, MAX_OPEN_FILES);
  
  /* Environment setup */
  env_data = (BYTE *)(g_env_segment<<4);
  strcpy(env_data, "PATH=.");
  env_data[7] = 0;
  *((WORD *)(env_data+8)) = 1; /* Count of the env-strings */
  /* NOTE: filename is only the filepath */
  strcpy(env_data + 10, filename);
#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("[DPMI] ENVSEG: %x - %x %x %x %s\n", (int)g_env_segment, env_data[0], env_data[1], env_data[2], env_data+10);
#endif

  /* Deprecated FD32 PSP members */
  /* npsp->info_sel = stubinfo_sel; */
  /* npsp->old_stack = current_SP; */
  /* npsp->memlimit = base + end; */
}

/* NOTE: Move the structure here, Correct? */
struct stubinfo {
  char magic[16];
  DWORD size;
  DWORD minstack;
  DWORD memory_handle;
  DWORD initial_size;
  WORD minkeep;
  WORD ds_selector;
  WORD ds_segment;
  WORD psp_selector;
  WORD cs_selector;
  WORD env_size;
  char basename[8];
  char argv0[16];
  char dpmi_server[16];
  /* FD/32 items */
  DWORD dosbuf_handler;
};

/* NOTE: FD32 module can't export ?_init name so make it internal with prefix `_' */
WORD _stubinfo_init(DWORD base, DWORD initial_size, DWORD mem_handle, char *filename, char *args, WORD cs_sel, WORD ds_sel)
{
  struct stubinfo *info;
  struct psp *newpsp;
  BYTE *allocated;
  DWORD size, env_size;
  int stubinfo_selector, psp_selector, env_selector;

  /*Environment lenght + 2 zeros + 1 word + program name... */
  env_size = 2 + 2 + strlen(filename) + 1;
  size = sizeof(struct stubinfo) + sizeof(struct psp);
  /* Always allocate a fixed amount of memory... */
  allocated = (BYTE *)mem_get(size);
  if (allocated == NULL) {
    return NULL;
  }

  info = (struct stubinfo *)allocated;
  stubinfo_selector = fd32_allocate_descriptors(1);
#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("[DOSEXEC] StubInfo Selector Allocated: = 0x%x\n", stubinfo_selector);
#endif
  if (stubinfo_selector == ERROR_DESCRIPTOR_UNAVAILABLE) {
    mem_free((DWORD)allocated, size);
    return NULL;
  }
  fd32_set_segment_base_address(stubinfo_selector, (DWORD)info);
  fd32_set_segment_limit(stubinfo_selector, sizeof(struct stubinfo));

  newpsp = (struct psp *)(allocated + sizeof(struct stubinfo));
  psp_selector = fd32_allocate_descriptors(1);
#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("[DOSEXEC] PSP Selector Allocated: = 0x%x\n", psp_selector);
#endif
  if (psp_selector == ERROR_DESCRIPTOR_UNAVAILABLE) {
    mem_free((DWORD)allocated, size);
    fd32_free_descriptor(stubinfo_selector);
    return NULL;
  }
  fd32_set_segment_base_address(psp_selector, (DWORD)newpsp);
  fd32_set_segment_limit(psp_selector, sizeof(struct psp));

  /* Allocate Environment Selector */
  env_selector = fd32_allocate_descriptors(1);
#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("[DOSEXEC] Environment Selector Allocated: = 0x%x\n", env_selector);
#endif
  if (env_selector == ERROR_DESCRIPTOR_UNAVAILABLE) {
    mem_free((DWORD)allocated , size);
    fd32_free_descriptor(stubinfo_selector);
    fd32_free_descriptor(psp_selector);
    return NULL;
  }
  fd32_set_segment_base_address(env_selector, g_env_segment<<4);
  fd32_set_segment_limit(env_selector, ENV_SIZE);

  strcpy(info->magic, "go32stub, v 2.02");
  info->size = sizeof(struct stubinfo);
  info->minstack = 0x40000;      /* FIXME: Check this!!! */
  info->memory_handle = mem_handle;
  /* Memory pre-allocated by the kernel... */
  info->initial_size = initial_size; /* align? */
  info->minkeep = 0x1000;        /* DOS buffer size... */
  info->dosbuf_handler = dosmem_get(0x1010);
  info->ds_segment = info->dosbuf_handler >> 4;
  info->ds_selector = ds_sel;
  info->cs_selector = cs_sel;
  info->psp_selector = psp_selector;
  /* TODO: There should be some error down here... */
  info->env_size = env_size;
  info->basename[0] = 0;
  info->argv0[0] = 0;
  memcpy(info->dpmi_server, "FD32.BIN", sizeof("FD32.BIN"));

  local_set_psp(newpsp, 0xFFFF/* fake, above 1M */, 0, env_selector, stubinfo_selector, g_fcb1, g_fcb2, filename, args);
  
  return stubinfo_selector;
}

void restore_psp(void)
{
  WORD stubinfo_sel;
  DWORD base, base1;
  int res;
  struct stubinfo *info;
  struct process_info *cur_P = fd32_get_current_pi();
  struct psp *curpsp = cur_P->psp;

  /* Free memory & selectors... */
  stubinfo_sel = curpsp->stubinfo_sel;
  fd32_get_segment_base_address(stubinfo_sel, &base);
#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("[DOSEXEC] Stubinfo Sel: 0x%x --- 0x%lx\n", stubinfo_sel, base);
#endif
  info = (struct stubinfo *)base;
  fd32_get_segment_base_address(info->psp_selector, &base1);
  if (base1 != (DWORD)curpsp) {
    error("Restore PSP: paranoia check failed...\n");
    message("Stubinfo Sel: 0x%x; Base: 0x%lx\n", stubinfo_sel, base);
    message("Current psp address: 0x%lx; Base1: 0x%lx\n", (DWORD)curpsp, base1);
    fd32_abort();
  }
  fd32_free_descriptor(stubinfo_sel);
  fd32_free_descriptor(info->psp_selector);
  fd32_free_descriptor(curpsp->ps_environ);

  fd32_free_jft(cur_P->jft, cur_P->jft_size);

  res = dosmem_free(info->dosbuf_handler, 0x1010);
  if (res != 1) {
    error("Restore PSP panic while freeing DOS memory...\n");
    fd32_abort();
  }
  
  res = mem_free(base, sizeof(struct stubinfo) + sizeof(struct psp) + ENV_SIZE);
  if (res != 1) {
    error("Restore PSP panic while freeing memory...\n");
    fd32_abort();
  }

  /* TODO: return frame OK?
  if (curpsp != NULL) {
    current_SP = curpsp->old_stack;
  } else {
    current_SP = NULL;
  }
  current_SP = curpsp->old_stack; */
}

static DWORD funky_base;
static WORD user_cs, user_ds;
static int wrapper_alloc_region(DWORD base, DWORD size)
{
  message("Wrapper allocating region: 0x%lx, 0x%lx...\n",
		  base, size);
  funky_base = base;

  return -1;
}

static DWORD wrapper_alloc(DWORD size)
{
  DWORD tmp;
  int data_selector, code_selector;
  
  if (funky_base == 0)
    return mem_get(size);

  message("Wrapper allocating 0x%lx = 0x%lx + 0x%lx...\n", size + funky_base, size, funky_base);
  /* NOTE: DjLibc doesn't need the memory address to be above the image base anymore */
  if ((tmp = mem_get(size + funky_base)) == 0)
    return 0;

  /* NOTE: The DS access rights is different with CS */
  if ((data_selector = fd32_allocate_descriptors(1)) == ERROR_DESCRIPTOR_UNAVAILABLE) {
    message("Cannot allocate data selector!\n");
    return 0;
  } else {
    fd32_set_segment_base_address(data_selector, (DWORD)tmp);
    fd32_set_segment_limit(data_selector, size + funky_base);
    fd32_set_descriptor_access_rights(data_selector, 0xC092);
  }

  if ((code_selector = fd32_allocate_descriptors(1)) == ERROR_DESCRIPTOR_UNAVAILABLE) {
    fd32_free_descriptor(data_selector);
    message("Cannot allocate code selector!\n");
    return 0;
  } else {
    fd32_set_segment_base_address(code_selector, (DWORD)tmp);
    fd32_set_segment_limit(code_selector, size + funky_base);
    fd32_set_descriptor_access_rights(code_selector, 0xC09A);
  }

  tmp += funky_base;
  funky_base = 0;

  user_cs = code_selector;
  user_ds = data_selector;
  fd32_log_printf("User CS, DS = 0x%x, 0x%x\n", code_selector, data_selector);

  return tmp;
}

/* FIXME: Simplify ---> user_stack is probably not needed! */
static int wrapper_create_process(DWORD entry, DWORD base, DWORD size,
		DWORD user_stack, char *filename, char *args)
{
  WORD stubinfo_sel;
  int res;
  int wrap_run(DWORD, WORD, DWORD, WORD, WORD, DWORD);

#ifdef __WRAP_DEBUG__
  fd32_log_printf("[WRAP] Going to run 0x%lx, size 0x%lx\n",
		entry, size);
#endif
  /* HACKME!!! size + stack_size... */
  /* FIXME!!! WTH is user_stack (== base + size) needed??? */
  stubinfo_sel = _stubinfo_init(base, size, 0, filename, args, user_cs, user_ds);
  if (stubinfo_sel == 0) {
    error("Error! No stubinfo!!!\n");
    return -1;
  }
#ifdef __WRAP_DEBUG__
  fd32_log_printf("[WRAP] Calling run 0x%lx 0x%lx (0x%x 0x%x) --- 0x%lx\n",
		entry, size, user_cs, user_ds, user_stack);
#endif

  res = wrap_run(entry, stubinfo_sel, 0, user_cs, user_ds, user_stack);
#ifdef __WRAP_DEBUG__
  fd32_log_printf("[WRAP] Returned 0x%x: now restoring PSP\n", res);
#endif

  restore_psp();

  return res;
}

static int wrapper_exec_process(struct kern_funcs *kf, int f, struct read_funcs *rf,
		char *filename, char *args)
{
  struct process_info pi;
  int retval;
  int size;
  DWORD exec_space;
  DWORD entry;
  DWORD base;

  /* Use wrapper memory allocation to create separate segments */
  kf->mem_alloc = wrapper_alloc;
  kf->mem_alloc_region = wrapper_alloc_region;

  entry = fd32_load_process(kf, f, rf, &exec_space, &base, &size);
  if (entry == -1) {
    return -1;
  }

  fd32_set_current_pi(&pi);

  /* Note: use the real entry */
  retval = wrapper_create_process(entry, exec_space, size,
		(DWORD)dpmi_stack+DPMI_STACK_SIZE, filename, args);
  message("Returned: %d!!!\n", retval);
  mem_free(exec_space, size);

  /* Back to the previous process NOTE: TSR native programs? */
  fd32_set_current_pi(pi.prev_P);

  return retval;
}


static int direct_exec_process(struct kern_funcs *kf, int f, struct read_funcs *rf,
		char *filename, char *args)
{
  struct process_info pi;
  int retval;
  int size;
  DWORD exec_space;
  DWORD entry;
  DWORD base;
  DWORD offset;
  WORD stubinfo_sel;

  entry = fd32_load_process(kf, f, rf, &exec_space, &base, &size);
  if (entry == -1) {
    return -1;
  }

  fd32_set_current_pi(&pi);

  if (exec_space == 0) {
    retval = fd32_create_process(entry, base, size, 0, filename, args);
  } else if (exec_space == -1) {
    /* create_dll(entry, base, size + LOCAL_BSS); */
    retval = 0;
  } else {
#ifdef __DOS_EXEC_DEBUG__
    fd32_log_printf("[DOSEXEC] Before calling 0x%lx...\n", entry);
#endif
    stubinfo_sel = _stubinfo_init(exec_space, size, 0 /* TODO: the DPMI memory handle to the image memory */, filename, args, get_cs(), get_ds());
    if (stubinfo_sel == 0) {
      error("No stubinfo!!!\n");
      return -1;
    }
    offset = exec_space - base;
    retval = fd32_create_process(entry + offset, exec_space, size, stubinfo_sel, filename, args);
#ifdef __DOS_EXEC_DEBUG__
    fd32_log_printf("[DOSEXEC] Returned 0x%x: now restoring PSP\n", retval);
#endif
    restore_psp();
    mem_free(exec_space, size);
  }
  /* Back to the previous process NOTE: TSR native programs? */
  fd32_set_current_pi(pi.prev_P);

  return retval;
}


static int vm86_call(WORD ip, WORD sp, X_REGS16 *in, X_REGS16 *out, X_SREGS16 *s)
{
  struct tss * p_vm86_tss = vm86_get_tss();

  if (p_vm86_tss->back_link != 0) {
    message("WTF? VM86 called with CS = 0x%x, IP = 0x%x\n", p_vm86_tss->cs, ip);
    fd32_abort();
  }
  /* Setup the stack frame */
  p_vm86_tss->ss = s->ss;
  p_vm86_tss->ebp = 0x91E; /* this is more or less random but some programs expect 0x9 in the high byte of BP!! */
  p_vm86_tss->esp = sp;

  /* Wanted VM86 mode + IOPL = 3! */
  p_vm86_tss->eflags = CPU_FLAG_VM | CPU_FLAG_IOPL | CPU_FLAG_IF;
  /* Ring0 system stack */
  p_vm86_tss->ss0 = X_FLATDATA_SEL;
  p_vm86_tss->esp0 = (DWORD)&dpmi_stack[DPMI_STACK_SIZE];

  /* Copy the parms from the X_*REGS structures in the vm86 TSS */
  p_vm86_tss->eax = (DWORD)in->x.ax;
  p_vm86_tss->ebx = (DWORD)in->x.bx;
  p_vm86_tss->ecx = (DWORD)in->x.cx;
  p_vm86_tss->edx = (DWORD)in->x.dx;
  p_vm86_tss->esi = (DWORD)in->x.si;
  p_vm86_tss->edi = (DWORD)in->x.di;
  /* IF Segment registers are required, copy them... */
  if (s != NULL) {
      p_vm86_tss->es = (WORD)s->es;
      p_vm86_tss->ds = (WORD)s->ds;
  } else {
      p_vm86_tss->ds = p_vm86_tss->ss;
      p_vm86_tss->es = p_vm86_tss->ss;
  }
  p_vm86_tss->gs = p_vm86_tss->ss;
  p_vm86_tss->fs = p_vm86_tss->ss;
  /* Execute the BIOS call, fetching the CS:IP of the real interrupt
   * handler from 0:0 (DOS irq table!)
   */
  p_vm86_tss->cs = s->cs;
  p_vm86_tss->eip = ip;

  /* Let's use the ll standard call... */
  p_vm86_tss->back_link = ll_context_save();
  
  if (out != NULL) {
    ll_context_load(X_VM86_TSS);
  } else {
    ll_context_to(X_VM86_TSS);
  }
  /* Back from the APP, through a software INT, see chandler.c */

  /* Send back in the X_*REGS structure the value obtained with
   * the real-mode interrupt call
   */
  if (out != NULL) {
    if (p_vm86_tss->back_link != 0) {message("Panic!\n"); fd32_abort();}

    out->x.ax = p_vm86_tss->eax;
    out->x.bx = p_vm86_tss->ebx;
    out->x.cx = p_vm86_tss->ecx;
    out->x.dx = p_vm86_tss->edx;
    out->x.si = p_vm86_tss->esi;
    out->x.di = p_vm86_tss->edi;
    out->x.cflag = p_vm86_tss->eflags;
  }
  if (s != NULL) {
    s->es = p_vm86_tss->es;
    s->ds = p_vm86_tss->ds;
  }
  return 1;
}

static int vm86_exec_process(struct kern_funcs *kf, int f, struct read_funcs *rf,
		char *filename, char *args)
{
  struct process_info pi;
  struct dos_header hdr;
  struct psp *ppsp;
  X_REGS16 in, out;
  X_SREGS16 s;
  DWORD mem;
  BYTE *exec_text;
  DWORD exec;
  DWORD exec_size;

  kf->file_read(f, &hdr, sizeof(struct dos_header));

  exec_size = hdr.e_cp*0x20-hdr.e_cparhdr+hdr.e_minalloc;

  mem = dosmem_get(sizeof(struct psp)+exec_size*0x10);
  /* NOTE: Align exec text at 0x10 boundary */
  if ((mem&0x0F) != 0) {
    message("[EXEC] Dos memory not at 0x10 boundary!\n");
  }

  ppsp = (struct psp *)mem;
  exec = mem+sizeof(struct psp);
  exec_text = (BYTE *)exec;

  kf->file_seek(f, hdr.e_cparhdr*0x10, kf->seek_set);
  kf->file_read(f, exec_text, exec_size*0x10);

#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("[DPMI] Exec at 0x%x: %x %x %x ... %x %x ...\n", (int)exec_text, exec_text[0], exec_text[1], exec_text[2], exec_text[0x100], exec_text[0x101]);
#endif
  /* Relocation */
  if (hdr.e_crlc != 0) {
    DWORD i;
    WORD *p, seg = exec>>4;
    struct dos_reloc *rel = (struct dos_reloc *)mem_get(sizeof(struct dos_reloc)*hdr.e_crlc);
    kf->file_seek(f, hdr.e_lfarlc, kf->seek_set);
    kf->file_read(f, rel, sizeof(struct dos_reloc)*hdr.e_crlc);

    for (i = 0; i < hdr.e_crlc; i++) {
      p = (WORD *)(((seg+rel[i].segment)<<4)+rel[i].offset);
      *p += seg;
    }
  }

#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("[DPMI] FCB: %x %x content: %x %x\n", (int)g_fcb1, (int)g_fcb2, *((BYTE *)g_fcb1), *((BYTE *)g_fcb2));
#endif

  s.ds = s.cs = (exec>>4)+hdr.e_cs;
  s.ss = s.cs+hdr.e_ss;
  s.es = (DWORD)ppsp>>4;
  in.x.ax = 0;
  in.x.bx = 0;
  in.x.dx = s.ds;
  in.x.di = hdr.e_sp;
  in.x.si = hdr.e_ip;
  fd32_log_printf("[DPMI] ES: %x DS: %x IP: %x\n", s.es, s.ds, hdr.e_ip);
  /* NOTE: Set the current process info */
  pi.name = filename;
  pi.args = args;
  pi.memlimit = 0;
  pi.cds_list = NULL; /* Pointer set by FS */
  fd32_set_current_pi(&pi);
  local_set_psp(ppsp, (exec+exec_size*0x10)>>4, 0, g_env_segment, 0, g_fcb1, g_fcb2, filename, args);
  /* Call in VM86 mode */
  vm86_call(hdr.e_ip, hdr.e_sp, &in, &out, &s);
  /* Back to the previous process */
  fd32_set_current_pi(pi.prev_P);
  dosmem_free(mem, sizeof(struct psp)+exec_size*0x10);

  return out.x.ax; /* Return value */
}

/* MZ format handling for VM86 */
static int isMZ(struct kern_funcs *kf, int f, struct read_funcs *rf)
{
  WORD magic;

  kf->file_read(f, &magic, 2);
  kf->file_seek(f, kf->file_offset, kf->seek_set);

  if (magic != 0x5A4D) { /* "MZ" */
    return 0;
  } else {
    return 1;
  }
}

/* MZ format handling for direct execution */
static int (*p_isMZ)(struct kern_funcs *kf, int f, struct read_funcs *rf) = NULL;
void (*dos_exec_mode16)(void) = NULL;

int dos_exec_switch(int option)
{
  int res = 1;

  if (g_env_segtmp == 0) {
    g_env_segtmp = dosmem_get(ENV_SIZE);
    g_env_segment = g_env_segtmp>>4;
  }
  switch(option)
  {
    case DOS_VM86_EXEC:
      /* dos_exec_mode16 */
      if (dos_exec_mode16 == NULL) {
        BYTE *p = (BYTE *)dosmem_get(0x10);
        /* ".code16" "mov $0xfd32, %ax;" "int $0x2f;" "retf;" */
        p[0] = 0xB8, p[1] = 0x32, p[2] = 0xFD;
        p[3] = 0xCD, p[4] = 0x2F, p[5] = 0xCB;
        dos_exec_mode16 = (void (*)(void))p;
      }
      /* Store the previous check */
      if (p_isMZ == NULL) {
        DWORD i;
        struct bin_format *binfmt = fd32_get_binfmt();
        for (i = 0; binfmt[i].name != NULL; i++)
          if (strcmp(binfmt[i].name, "mz") == 0) {
            p_isMZ = binfmt[i].check;
            break;
          }
      }
      fd32_set_binfmt("mz", isMZ, vm86_exec_process);
      break;
    case DOS_DIRECT_EXEC:
      fd32_set_binfmt("mz", p_isMZ, direct_exec_process);
      break;
    case DOS_WRAPPER_EXEC:
      fd32_set_binfmt("mz", p_isMZ, wrapper_exec_process);
      break;
    default:
      res = 0;
      break;
  }
  
  return res;
}

int dos_exec(char *filename, DWORD env_segment, char *args,
		DWORD fcb1, DWORD fcb2, WORD *return_val)
{
  struct kern_funcs kf;
  struct read_funcs rf;
  struct bin_format *binfmt;
  struct kernel_file f;
  DWORD i;

  if (fd32_kernel_open(filename, O_RDONLY, 0, 0, &f) < 0)
    return -1;

#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("FileId = 0x%lx (0x%lx)\n", (DWORD)f.file_id, (DWORD)&f);
#endif
  kf.file_read = fd32_kernel_read;
  kf.file_seek = fd32_kernel_seek;
  kf.mem_alloc = mem_get;
  kf.mem_alloc_region = mem_get_region;
  kf.mem_free = mem_free;
  kf.message = message;
  kf.log = fd32_log_printf;
  kf.error = message;
  kf.get_dll_table = get_dll_table;
  kf.add_dll_table = add_dll_table;
  kf.seek_set = FD32_SEEKSET;
  kf.seek_cur = FD32_SEEKCUR;
  kf.file_offset = 0;

  /* Get the binary format object table, ending with NULL name */
  binfmt = fd32_get_binfmt();
  
  /* Load different modules in various binary format */
  for (i = 0; binfmt[i].name != NULL; i++)
  {
    if (binfmt[i].check(&kf, (int)(&f), &rf)) {
      g_env_segment = env_segment;
      g_fcb1 = fcb1;
      g_fcb2 = fcb2;
      /* NOTE: args[0] is the length of the args */
      *return_val = binfmt[i].exec(&kf, (int)(&f), &rf, filename, args+1);
      break;
    }
#ifdef __DOS_EXEC_DEBUG__
    else {
      fd32_log_printf("[MOD] Not '%s' format\n", binfmt[i].name);
    }
#endif
    /* p->file_seek(file, p->file_offset, p->seek_set); */
  }

  fd32_kernel_close((int)&f);
  return 0;
}
