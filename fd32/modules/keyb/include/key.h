#ifndef __KEY_H__
#define __KEY_H__

#define KEYB_DEV_INFO     0x80

/* keyboard I/O ports */
#define KEYB_STATUS_PORT  0x64
#define KEYB_DATA_PORT    0x60
#define KEYB_CTL_TIMEOUT  0x10000
/* keyboard command */
#define KEYB_CMD_SET_LEDS 0xED

/* keyboard standard status (same with FD-KEYB) */
typedef union keyb_std_status
{
	WORD data;
	struct
	{
		BYTE rshift        :1;
		BYTE lshift        :1;
		BYTE ctrl          :1;
		BYTE alt           :1;
		BYTE scrlk_active  :1;
		BYTE numlk_active  :1;
		BYTE capslk_active :1;
		BYTE reserved_1    :1;
		BYTE lctrl         :1;
		BYTE lalt          :1;
		BYTE rctrl         :1;
		BYTE ralt          :1;
		BYTE e0_prefix     :1;
		BYTE reserved_2    :1;
		BYTE shift         :1;
		BYTE reserved_3    :1;
	} s;
} keyb_std_status_t;

int preprocess(BYTE code);
void postprocess(void);
void keyb_state_reset(void);
BYTE keyb_get_data(void);
BYTE keyb_get_status(void);
int keyb_send_cmd(BYTE cmd);
WORD keyb_get_shift_flags(void);
int keyb_set_shift_flags(WORD f);
WORD keyb_decode(BYTE c, keyb_std_status_t stdst);
void keyb_hook(WORD key, int isCTRL, int isALT, DWORD hook_func);
void keyb_fire_hook(WORD key, int isCTRL, int isALT);

/* make code */
#define MK_CTRL       0x1D
#define MK_LSHIFT     0x2A
#define MK_RSHIFT     0x36
#define MK_ALT        0x38
#define MK_CAPS       0x3A
#define MK_NUMLK      0x45
#define MK_SCRLK      0x46
#define MK_SYSRQ      0x54
#define MK_INSERT     0x52

#define BREAK         0x0080

/* flags - active */
#define ACT_SCRLK     0x0010
#define ACT_NUMLK     0x0020
#define ACT_CAPSLK    0x0040
#define ACT_INSERT    0x0080
/* flag */
#define FLAG_RSHIFT   0x0001
#define FLAG_LSHIFT   0x0002
#define FLAG_CTRL     0x0004
#define FLAG_ALT      0x0008
#define FLAG_LCTRL    0x0100
#define FLAG_LALT     0x0200
#define FLAG_SYSRQ    0x0400
#define FLAG_SUSPEND  0x0800
#define FLAG_SCRLK    0x1000
#define FLAG_NUMLK    0x2000
#define FLAG_CAPS     0x4000
#define FLAG_INSTERT  0x8000

/* mode */
#define MODE_E1       0x01
#define MODE_E0       0x02
#define MODE_RCTRL    0x04
#define MODE_RALT     0x08
#define MODE_EINST    0x10
#define MODE_FNUMLK   0x20
#define MODE_LASTID   0x40
#define MODE_READID   0x80

/* led */
#define LEGS_MASK     0x07
#define LED_SCRLK     0x01
#define LED_NUMLK     0x02
#define LED_CAPSLK    0x04
#define LED_CIRCUS    0x08
#define LED_ACKRCV    0x10
#define LED_RSRCV     0x20
#define LED_MODEUPD   0x40
#define LED_TRANSERR  0x80

#endif
