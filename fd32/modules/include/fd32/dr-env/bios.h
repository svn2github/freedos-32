#ifndef __FD32_DRENV_BIOS_H
#define __FD32_DRENV_BIOS_H

#include <ll/i386/hw-data.h>
#include <ll/i386/x-bios.h>

#define FD32_DECLARE_REGS(a) X_REGS16 regs_##a; \
                             X_SREGS16 selectors_##a

#define AL(a)    regs_##a.h.al
#define AH(a)    regs_##a.h.ah
#define BL(a)    regs_##a.h.bl
#define BH(a)    regs_##a.h.bh
#define CL(a)    regs_##a.h.cl
#define CH(a)    regs_##a.h.ch
#define DL(a)    regs_##a.h.dl
#define DH(a)    regs_##a.h.dh
#define AX(a)    regs_##a.x.ax
#define BX(a)    regs_##a.x.bx
#define CX(a)    regs_##a.x.cx
#define DX(a)    regs_##a.x.dx
#define CS(a)    selectors_##a.cs
#define DS(a)    selectors_##a.ds
#define ES(a)    selectors_##a.es
#define GS(a)    selectors_##a.gs
#define FS(a)    selectors_##a.fs
#define SS(a)    selectors_##a.ss
#define SP(a)    regs_##a.x.sp
#define IP(a)    regs_##a.x.ip
#define FLAGS(a) regs_##a.x.cflag
#define SI(a)    regs_##a.x.si
#define DI(a)    regs_##a.x.di
#define BP(a)    regs_##a.x.bp
#define EAX(a)   regs_##a.d.eax
#define EBX(a)   regs_##a.d.ebx
#define ECX(a)   regs_##a.d.ecx
#define EDX(a)   regs_##a.d.edx

#define fd32_int(n, r) do { \
  DWORD f; \
  f = ll_fsave(); \
  vm86_callBIOS(n, &regs_##r, &regs_##r, &selectors_##r); \
  ll_frestore(f); \
} while (0)

extern DWORD rm_irq_table[256];

/* BIOS data area */
extern struct bios_data_area {
  WORD com_baseaddr[4];    /* 0x400 */
  WORD lpt_baseaddr[4];
  WORD device_config;
  BYTE ikeyblink_errcount; /* PCjr */
  WORD mem_size;           /* in KB */
  BYTE reserved_1;
  BYTE ps2_bios_ctrl_flag;
  WORD keyb_flag;
  BYTE keyb_alttoken;
  WORD keyb_bufhead;
  WORD keyb_buftail;
  BYTE keyb_buf[32];
  BYTE drive_active;
  BYTE drive_running;
  BYTE disk_motor_timeout;
  BYTE disk_status;
  BYTE fdc_ctrl_status[7];
  BYTE video_mode;
  WORD video_column_size;
  WORD video_membuf_size;
  WORD video_page_offset;
  WORD video_cursor_pos[8];
  WORD video_cursor_shape;
  BYTE video_active_page;
  WORD video_crtc_port;
  BYTE vdu_ctrl_reg;
  BYTE vdu_color_reg;
  BYTE temp_storage[5];
  DWORD timer;
  BYTE clock_rollover_flag;
  BYTE ctrl_break_flag;
  WORD soft_reset_flag;
  BYTE harddisk_lastop_status;
  BYTE harddisk_count;
  BYTE fixeddisk_ctrl_byte; /* XT */
  BYTE fixeddisk_port;
  BYTE lpt_timeout[4];
  BYTE com_timeout[4];
  WORD keyb_bufstart;
  WORD keyb_bufend;
  BYTE video_row_size;
  WORD video_font_height;
  BYTE video_mode_options;
  BYTE video_feat_bit;
  BYTE video_display_data;
  BYTE video_dcc_index;
  BYTE disk_last_data_rate;
  BYTE harddisk_status;
  BYTE harddisk_error;
  BYTE harddisk_int_ctrl_flag;
  BYTE disk_combo_card;
  BYTE disk_media_state[4];
  BYTE disk_track[2];
  BYTE keyb_mode;
  BYTE keyb_led;
  DWORD pointer_wait_flag;
  DWORD user_wait_count;
  BYTE rtc_wait_flag;
  BYTE lana_dma_channel_flag;
  BYTE lana_status[2];
  DWORD harddisk_int_vector;
  DWORD video_saveptr;
  BYTE reserved_2[8];
  BYTE keyb_nmi_ctrl_flag;
  DWORD keyb_break_pending_flag;
  BYTE keyb_byte_queue_port;
  BYTE keyb_last_scancode;
  BYTE keyb_nmi_bufhead;
  BYTE keyb_nmi_buftail;
  BYTE keyb_nmi_scancode_buf[16];
  BYTE reserved_3;
  WORD alt_day_count;
  BYTE reserved_4[32];
  BYTE intra_comm_area[16];
} __attribute__ ((packed)) bios_da;

#endif /* #ifndef __FD32_DRENV_BIOS_H */

