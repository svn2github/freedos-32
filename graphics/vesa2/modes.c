/**************************************************************************
 * FreeDOS32 VESA2 Graphics Library                                       *
 * by Salvo Isaja                                                         *
 *                                                                        *
 * Copyright (C) 2003, Salvatore Isaja, all rights reserved.              *
 *                                                                        *
 * This is "modes.c" - Setting VESA2 video modes.                         *
 *                                                                        *
 *                                                                        *
 * This file is part of the FreeDOS32 VESA2 Graphics Library (LIBRARY).   *
 *                                                                        *
 * The LIBRARY is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the  *
 * Free Software Foundation; either version 2 of the License, or (at your *
 * option) any later version.                                             *
 *                                                                        *
 * The LIBRARY is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 * GNU General Public License for more details.                           *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with the LIBRARY; see the file COPYING.txt; if not, write to     *
 * the Free Software Foundation, Inc.,                                    *
 * 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA                *
 **************************************************************************/
#include "vesa2.h"
#include <sys/nearptr.h>
#include <sys/movedata.h>
#include <dpmi.h>

static __dpmi_meminfo vesa2_mi;


/* Retrieves informations on the specified video mode in the info structure */
int vesa2_get_mode_info(WORD mode, VesaModeInfo *info)
{
  int         seg, sel; /* Segment and selector for low-mem VESA2 info */
  __dpmi_regs r;

  seg = __dpmi_allocate_dos_memory((sizeof(VesaModeInfo) + 15) >> 4 , &sel);
  if (seg < 0) return -1;
  r.x.ax = 0x4F01; /* VBE2 service 01h: Get Mode Info */
  r.x.cx = mode;
  r.x.di = 0;
  r.x.es = seg;
  if (__dpmi_int(0x10, &r))
  {
    __dpmi_free_dos_memory(sel);
    return -1;
  }
  if (r.h.ah)
  {
    __dpmi_free_dos_memory(sel);
    return -1;
  }
  dosmemget(seg << 4, sizeof(VesaModeInfo), info);
  __dpmi_free_dos_memory(sel);
  return 0;
}


/* Search for a specified video mode.                              */
/* If found, returns the mode number and fills the info structure. */
/* Only Linear Frame Buffer modes are supported.                   */
int vesa2_find_video_mode(unsigned x_res, unsigned y_res, unsigned bpp,
                          VesaModeInfo *info)
{
  unsigned     mode;
  VesaModeInfo i;

  if (!info) return -1;
  for (mode = 0x100; mode < 0x200; mode++)
    if (vesa2_get_mode_info(mode, &i) == 0)
      if ((i.x_res          == x_res)
       && (i.y_res          == y_res)
       && (i.bits_per_pixel == bpp)
       && (i.mode_attributes & VMI_MA_LFB))
      {
        *info = i;
        return mode;
      }
  return -1; /* Mode not available */
}


/* Set the video mode to the specified one, where mode is the desired mode */
/* number and info the VESA the info block of that mode, both achieved by  */
/* vesa2_find_video_mode.                                                  */
/* On success stores a pointer to the linear frame buffer in lfb.          */
/* Only Linear Frame Buffer modes are used.                                */
int vesa2_set_video_mode(WORD mode, const VesaModeInfo *info, void **lfb)
{
  __dpmi_regs r;

  if (!lfb) return -1;
  if (!(info->mode_attributes & VMI_MA_SUPPORTED)) return -1; /* Mode not supported */
  if (!(info->mode_attributes & VMI_MA_LFB)) return -1; /* LFB mode not available */
  r.x.ax = 0x4F02;  /* VBE2 service 02h: Set VBE Mode   */
  mode  |= 1 << 14; /* Select bit 14 for LFB addressing */
  r.x.bx = mode;
  __dpmi_int(0x10, &r);
  if (r.x.ax != 0x004F) return -1; /* Error entering mode */
  r.x.ax = 0x4F03; /* VBE2 service 03h: Return current VBE Mode */
  __dpmi_int(0x10, &r);
  if (r.x.ax != 0x004F) return -1;
  if (r.x.bx != mode) return -1; /* Mode was not set correctly */
  /* Map LFB physical address to lineary memory and lock it */
  vesa2_mi.handle  = 0;
  vesa2_mi.size    = info->x_res * info->y_res * info->bits_per_pixel;
  vesa2_mi.address = info->phys_base_ptr;
  __dpmi_physical_address_mapping(&vesa2_mi);
  __dpmi_lock_linear_region(&vesa2_mi);
  vesa2_set_clipping(0, info); /* Disable clipping */
  #if 0
  /* Allocate a separate descriptor for the Linear Buffer */
  int selector = __dpmi_allocate_ldt_descriptors(1);
  __dpmi_set_segment_base_address(selector, vesa2_mi.address);
  __dpmi_set_segment_limit(selector, vesa2_mi.size - 1);
  #else
  /* Point to the Linear Frame Buffer using the Fat DS */
  *lfb = (BYTE *) vesa2_mi.address + __djgpp_conventional_base;
  #endif
  return 0;
}


/* Restore video mode 3 (standard 80 columns color text mode) */
void vesa2_done(void)
{
  __dpmi_regs r;

  r.x.ax = 0x0003;
  __dpmi_int(0x10, &r);

  /* Unlock linear memory for LFB and release physycal address mapping */
  __dpmi_unlock_linear_region(&vesa2_mi);
  __dpmi_free_physical_address_mapping(&vesa2_mi);
}
