/* FD/32 Initialization Code
 * by Luca Abeni
 *
 * This is free software; see GPL.txt
 */
#include <ll/i386/error.h>
#include <ll/i386/mem.h>
#include <ll/i386/mb-info.h>

#include "init.h"
#include "kernel.h"
#include "kmem.h"
#include "pmm.h"
#include "mods.h"

void system_init(void *mbp)
{
  /* The interrupt handlers are in the drivers, now!!! */
  /* (and exception handlers are set by kernel_init() */
  mem_init(mbp);
  kernel_init();
}

void dos_init(void *p)
{
  struct multiboot_info *mbp;
  DWORD membase, memsize;

  mbp = p;
  if (mbp->flags & MB_INFO_MEMORY) {
#ifdef __MEM_DEBUG__
    message("\tDOS: Memory information OK\n");
#endif
    memsize = mbp->mem_lower * 1024;
#ifdef __MEM_DEBUG__
    message("DOS Mem Size: %lx %lu\n", memsize, memsize);
#endif
    /* NOTE: Do not reserve for the GRUB data (formerly Randomly chosen safe value 0x10000) */
    membase = 0x400+0x100; /* Just reserve for the real-mode interrupts jump table and BDA */
    if (mbp->flags & MB_INFO_BOOT_LOADER_NAME) {
      if (*((char *)(mbp->boot_loader_name)) == 'X') {
        membase = mbp->mem_lowbase;
      }
    }
    /* Reserve the memory before membase */
    memsize -= membase;

    dosmem_init(membase, memsize);
    /* NOTE: Do not reserve the module structure infomation and it's in DOS memory
    dosmem_get_region(mbp->mods_addr, sizeof(struct mods_struct)*mbp->mods_count); */
  } else {
    message("Cannot initialize the DOS Memory Pool\n");
  }
}
