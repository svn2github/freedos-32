/* Keyborad Driver for FD32
 * original by Luca Abeni
 * extended by Hanzac Chen
 *
 * 2004 - 2005
 * This is free software; see GPL.txt
 */


/* scan code */
#define VK_ESCAPE     0x1B

/* make code */
#define MK_LCTRL      0x1D
#define MK_LSHIFT     0x2A
#define MK_RSHIFT     0x36
#define MK_LALT       0x38
#define MK_CAPS       0x3A
#define MK_NUMLK      0x45
#define MK_SCRLK      0x46
#define MK_SYSRQ      0x54
#define MK_RCTRL      0xE01D
#define MK_RALT       0xE038
#define MK_INSERT     0xE052

#define BREAK         0x0080

/* led */
#define LED_SCRLK     0x0040
#define LED_NUMLK     0x0020
#define LED_CAPS      0x0010

/* flag */
#define RSHIFT_FLAG   0x0001
#define LSHIFT_FLAG   0x0002
#define CTRL_FLAG     0x0004
#define ALT_FLAG      0x0008
#define INSERT_FLAG   0x0080

#define LCTRL_FLAG    0x0100
#define LALT_FLAG     0x0200
#define RCTRL_FLAG    0x0400
#define RALT_FLAG     0x0800
#define SCRLK_FLAG    0x1000
#define NUMLK_FLAG    0x2000
#define CAPS_FLAG     0x4000
#define SYSRQ_FLAG    0x8000
