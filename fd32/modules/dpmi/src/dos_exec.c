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
#include "dos_exec.h"

#define __DOS_EXEC_DEBUG__


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
  p_vm86_tss->eflags = CPU_FLAG_VM | CPU_FLAG_IOPL;
  /* Ring0 system stack */
  p_vm86_tss->ss0 = X_FLATDATA_SEL;
  p_vm86_tss->esp0 = mem_get(VM86_STACK_SIZE)+VM86_STACK_SIZE-1;

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
  } /* else {
    ll_context_to(X_VM86_TSS);
  } */
  /* Back from the APP, through a software INT, see chandler.c */

  /* Send back in the X_*REGS structure the value obtained with
   * the real-mode interrupt call
   */
  if (out != NULL) {
    if (p_vm86_tss->back_link != 0) {message("Panic!\n"); fd32_abort();}

    /* out->x.ax = global_regs->eax;
    out->x.bx = global_regs->ebx;
    out->x.cx = global_regs->ecx;
    out->x.dx = global_regs->edx;
    out->x.si = global_regs->esi;
    out->x.di = global_regs->edi;
    out->x.cflag = global_regs->flags; */
  }
  if (s != NULL) {
    s->es = p_vm86_tss->es;
    s->ds = p_vm86_tss->ds;
  }
  return 1;
}

/* File handling */
struct funky_file {
  fd32_request_t *request;
  void *file_id;
};

static int my_read(int id, void *b, int len)
{
  struct funky_file *f;
  fd32_read_t r;
  int res;

  f = (struct funky_file *)id;
  r.Size = sizeof(fd32_read_t);
  r.DeviceId = f->file_id;
  r.Buffer = b;
  r.BufferBytes = len;
  res = f->request(FD32_READ, &r);
#ifdef __DEBUG__
  if (res < 0) {
    fd32_log_printf("WTF!!!\n");
  }
#endif

  return res;
}

static int my_seek(int id, int pos, int w)
{
  int error;
  struct funky_file *f;
  fd32_lseek_t ls;

  f = (struct funky_file *)id;
  ls.Size = sizeof(fd32_lseek_t);
  ls.DeviceId = f->file_id;
  ls.Offset = (long long int) pos;
  ls.Origin = (DWORD) w;
  error = f->request(FD32_LSEEK, &ls);
  return (int) ls.Offset;
}

/* TODO: Re-consider the fcb1 and fcb2 to support multi-tasking */
static DWORD g_fcb1 = 0, g_fcb2 = 0, g_env_segment, g_env_segtmp = 0;
static int vm86_exec_process(struct kern_funcs *kf, int f, struct read_funcs *rf,
		char *cmdline, char *args)
{
  struct dos_header hdr;
  struct psp *ppsp;
  X_REGS16 in, out;
  X_SREGS16 s;
  DWORD mem;
  BYTE *exec_text;
  BYTE *env_data;
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
  env_data = (BYTE *)(g_env_segment<<4);
  env_data[0] = 0;
  env_data[1] = 1;
  env_data[2] = 0;
  /* NOTE: cmdline is only the filepath */
  strcpy(env_data + 3, cmdline);
#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("[DPMI] ENVSEG: %x - %x %x %x %s\n", (int)g_env_segment, env_data[0], env_data[1], env_data[2], env_data+3);
#endif
  /* Init PSP */
  ppsp->ps_size = (exec+exec_size*0x10)>>4;
  ppsp->ps_parent = 0;
  ppsp->ps_environ = g_env_segment;
  ppsp->ps_maxfiles = 0x20;
  ppsp->ps_filetab = fd32_init_jft(0x20);
  if (g_fcb1 != NULL) {
    g_fcb1 = (g_fcb1>>12)+(g_fcb1&0x0000FFFF);
    memcpy(ppsp->def_fcb_1, (void *)g_fcb1, 16);
  }
  if (g_fcb2 != NULL) {
    g_fcb2 = (g_fcb2>>12)+(g_fcb2&0x0000FFFF);
    memcpy(ppsp->def_fcb_2, (void *)g_fcb2, 20);
  }
  if (args != NULL) {
    ppsp->command_line_len = strlen(args);
    strcpy(ppsp->command_line, args);
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

  /* NOTE: Set the current_psp */
  extern struct psp *current_psp;
  current_psp = ppsp;
  /* Call in VM86 mode */
  vm86_call(hdr.e_ip, hdr.e_sp, &in, &out, &s);
  dosmem_free(mem, sizeof(struct psp)+exec_size*0x10);
  
  return 0;
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
  int res = 0;

  switch(option)
  {
    case DOS_VM86_EXEC:
      if (g_env_segtmp == 0) {
        g_env_segtmp = dosmem_get(0x100);
        g_env_segment = g_env_segtmp>>4;
      }
      /* dos_exec_mode16 */
      if (dos_exec_mode16 == NULL) {
        BYTE *p = (BYTE *)dosmem_get(0x10);
        /* ".code16" "mov $0xfd32, %ax;" "int $0x2f;" "retf;" */
        p[0] = 0xB8, p[1] = 0x32, p[2] = 0xFD;
        p[3] = 0xCD, p[4] = 0x2F, p[5] = 0xCB;
        dos_exec_mode16 = (void (*)(void))p;
      }
      if (p_isMZ == NULL) {
        DWORD i;
        struct bin_format *binfmt = fd32_get_binfmt();
        for (i = 0; binfmt[i].name != NULL; i++)
          if (strcmp(binfmt[i].name, "mz") == 0)
            p_isMZ = binfmt[i].check;
      }
      fd32_set_binfmt("mz", isMZ, vm86_exec_process);
      res = 1;
      break;
    case DOS_DIRECT_EXEC:
      fd32_set_binfmt("mz", p_isMZ, fd32_exec_process);
      res = 1;
      break;
    default:
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
  struct funky_file f;
  void *fs_device;
  char *pathname;
  fd32_openfile_t of;
  DWORD i;

/* TODO: filename must be canonicalized with fd32_truename, but fd32_truename
         resolve the current directory, that is per process.
         Have we a valid current_psp at this point?
         Next, truefilename shall be used instead of filename.
  char truefilename[FD32_LFNPMAX];
  if (fd32_truename(truefilename, filename, FD32_TNSUBST) < 0) {
#ifdef __DEBUG__
    fd32_log_printf("Cannot canonicalize the file name!!!\n");
#endif
    return -1;
  } */
  if (fd32_get_drive(/*true*/filename, &f.request, &fs_device, &pathname) < 0) {
#ifdef __DOS_EXEC_DEBUG__
    fd32_log_printf("Cannot find the drive!!!\n");
#endif
    return -1;
  }
#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("Opening %s\n", /*true*/filename);
#endif
  of.Size = sizeof(fd32_openfile_t);
  of.DeviceId = fs_device;
  of.FileName = pathname;
  of.Mode = O_RDONLY;
  if (f.request(FD32_OPENFILE, &of) < 0) {
#ifdef __DOS_EXEC_DEBUG__
    fd32_log_printf("File not found!!\n");
#endif
    return -1;
  }
  f.file_id = of.FileId;

#ifdef __DOS_EXEC_DEBUG__
  fd32_log_printf("FileId = 0x%lx (0x%lx)\n", (DWORD)f.file_id, (DWORD)&f);
#endif
  kf.file_read = my_read;
  kf.file_seek = my_seek;
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

  /* TODO: Close file */
  return 1;
}
