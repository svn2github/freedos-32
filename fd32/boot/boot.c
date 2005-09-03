/* FD/32 Initialization Code
 * by Luca Abeni
 * 
 * This is free software; see GPL.txt
 */

#include <ll/i386/stdlib.h>
#include <ll/i386/cons.h>
#include <ll/i386/error.h>
#include <ll/i386/mem.h>
#include <ll/i386/mb-info.h>
#include <ll/i386/hw-instr.h>
#include <ll/i386/hw-func.h>
#include <ll/i386/x-bios.h>

#include <ll/sys/ll/ll-func.h>
#include <ll/sys/ll/time.h>
#include <ll/sys/ll/event.h>

#include <ll/string.h>

/* #define STATIC_DPMI */
/* #define STATIC_BIOSDISK */
#define VM86_INIT 1
#define VM86_STACK_SIZE 0x2000
#define __BOOT_DEBUG__
#define DISK_TEST 1

#include "params.h"
#include "format.h"
#include "logger.h"
#include "init.h"
#include "mods.h"
#include "mmap.h"
#include "filesys.h"	/* For fd32_set_boot_device() */
#include "kmem.h"

#ifdef STATIC_DPMI
extern void DPMI_init(void);
#endif
#ifdef STATIC_BIOSDISK
extern void biosdisk_init(void);
#endif

static int timer_mode = LL_PERIODIC;
static int timer_period = 1000;

static void Timer_set(char *value)
{
  int p;
  
  if (value != NULL) {
    p = atoi(value);
    if ((p > 0) && (p <= 50000)) {
      message("Setting Tick = %d\n", p);
      timer_period = p;
    } else {
      message("Wrong Tick value %d; ignoring it\n", p);
    }
  } else {
    message("Warning: option \"--tick\" needs an argument; ignoring it\n");
  }
}

void args_parse(int argc, char *argv[])
{
  char *name, *value = NULL;
  int i, j;

  add_param("coff", COFF_set_type);
  add_param("tick", Timer_set);

  for (i = 1; i < argc; i++) {
    /* kernel parameters ...
     *    --paramname=paramval
     * or --paramname paramval
     */
    if (*((WORD *)argv[i]) == *((WORD *)"--")) {
      /* Get the param name and value */
      name = argv[i]+2;
      for (j = 2 ; argv[i][j] != '='; j++) {
        if (argv[i][j] == 0) {
          i++;
          if (i < argc) {
            value = argv[i];
            break;
          } else {
            set_param(name, NULL);
            return;
          }
        }
      }
      if (value == NULL) {
        argv[i][j] = 0;
        value = argv[i]+j+1;
      }
      /* Set the param */
      set_param(name, value);
      value = NULL;
    }
  }
}

int main (int argc, char *argv[])
{
#ifdef __BOOT_DEBUG__
  DWORD sp1, sp2;
#endif
  struct ll_initparms parms;
  struct multiboot_info *mbi;
  DWORD lbase = 0, hbase = 0;
  DWORD lsize = 0, hsize = 0;
  extern WORD kern_CS, kern_DS;
  
#ifdef __BOOT_DEBUG__
  sp1 = get_sp();
#endif

  cli();
  args_parse(argc, argv);
  /*FIXME: Use a boot time parameter to decide if LL_FORCE_TIMER2 or not...*/
  parms.mode = timer_mode | LL_FORCE_TIMER2;
  parms.tick = timer_period;
  mbi = ll_init();
  event_init(&parms);

  /* Set the kern_CS and kern_DS for DPMI... */
  kern_CS = get_cs();
  kern_DS = get_ds();

  message("Kernel DS: 0x%x   Kernel CS: 0x%x\n", kern_CS, kern_DS);

  if (mbi == NULL) {
	message("Error in LowLevel initialization code...\n");
	sti();
	l1_exit(-1);
  }
  sti();

  message("FreeDOS/32 Kernel booting...\n");
  message("System information from multiboot (flags 0x%lx):\n",
	  mbi->flags);

  if (mbi->flags & MB_INFO_MEMORY) {
#ifdef __BOOT_DEBUG__
    message("[FD32] Memory information OK\n");
#endif
    lsize = mbi->mem_lower * 1024;
    hsize = mbi->mem_upper * 1024;
#ifdef __BOOT_DEBUG__
    message("    Mem Lower: %lx %lu\n", lsize, lsize);
    message("    Mem Upper: %lx %lu\n", hsize, hsize);
#endif
    lbase = 0x0;
    hbase = 0x100000;
    if (mbi->flags & MB_INFO_BOOT_LOADER_NAME) {
#ifdef __BOOT_DEBUG__
      message("Loader Name provided: %s\n", (char *)mbi->boot_loader_name);
#endif
      if (*((char *)(mbi->boot_loader_name)) == 'X') {
        lbase = mbi->mem_lowbase;
        hbase = mbi->mem_upbase;
      }
    }
#ifdef __BOOT_DEBUG__
    message("        Low Memory: %ld - %ld (%lx - %lx) \n", 
	    lbase, lbase + lsize,
	    lbase, lbase + lsize);
    message("        High Memory: %ld - %ld (%lx - %lx)\n", 
	    hbase, hbase + hsize,
	    hbase, hbase + hsize);
#endif
  }
  
  system_init(mbi);
  
  if (mbi->flags & MB_INFO_BOOTDEV) {
    fd32_set_boot_device(mbi->boot_device);
#ifdef __BOOT_DEBUG__
    message("    Boot device set: %08lx\n", mbi->boot_device);
#endif
  }
  
  dos_init(mbi);

  fd32_log_init();
#ifdef VM86_INIT
  /* NOTE: Pre-initialize VM86 for biosdisk, DPMI, ... */
  vm86_init((BYTE *)dosmem_get(VM86_STACK_SIZE), VM86_STACK_SIZE);
#endif
#ifdef STATIC_BIOSDISK
  biosdisk_init();
#endif
#ifdef STATIC_DPMI
  DPMI_init();
#endif
  
  if (mbi->flags & MB_INFO_MODS) {
#ifdef __BOOT_DEBUG__
    fd32_log_printf("[FD32] Attempting to load the shell...\n");
#endif
    process_modules(mbi->mods_count, mbi->mods_addr);
  } else {
    message("Sorry, no modules provided... I cannot load the shell!!!\n");
  }

#ifdef STATIC_BIOSDISK
#if (DISK_TEST == 1)
  bios_test();
#endif

#if (DISK_TEST == 2)
  disk_test();
#endif

#if (DISK_TEST == 3)
  fat_test();
#endif
#endif
  cli();
  l1_end();
#ifdef __BOOT_DEBUG__
  sp2 = get_sp();
  fd32_log_printf("[FD32] End reached!\n");
  fd32_log_printf("    Actual stack : %lx - ", sp2);
  fd32_log_printf("    Begin stack : %lx\n", sp1);
  fd32_log_printf("    Check if same : %s\n",sp1 == sp2 ? "Ok :-)" : "No :-(");
#endif    
  fd32_log_stats(NULL, NULL);
  return 1;
}
