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

#include <ll/sys/ll/ll-func.h>
#include <ll/sys/ll/time.h>
#include <ll/sys/ll/event.h>

#include <ll/string.h>

/* #define STATIC_DPMI */
/* #define STATIC_BIOSDISK */

#define DISK_TEST 1

#include "logger.h"
#include "init.h"
#include "mods.h"
#include "mmap.h"
#include "filesys.h"	/* For fd32_set_boot_device() */

#ifdef STATIC_DPMI
extern void DPMI_init(void);
#endif
#ifdef STATIC_BIOSDISK
extern void biosdisk_init(void);
#endif

static int timer_mode = LL_PERIODIC;
static int timer_period = 1000;

void args_parse(int argc, char *argv[])
{
  int i;

  i = 1;
  while (i < argc) {
    if (strcmp(argv[i], "--tick") == 0) {
      int p;
      
      i++;
      if (i < argc) {
        p = atoi(argv[i]);
        if ((p > 0) && (p < 50000)) {
	  message("Setting Tick = %d\n", p);
	  timer_period = p;
        } else {
	  message("Wrong Tick value %d; ignoring it\n", p);
        }
      } else {
	message("Error: otpion \"--tick\" needs an argument; ignoring it\n");
      }
    }

    i++;
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
    message("    Boot device set: %u %u %u %u %08x\n", b[0], b[1], b[2], b[3], mbi->boot_device);
#endif
  }
  
  dos_init(mbi);

  fd32_log_init();
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
  fd32_log_stats();
  return 1;
}
