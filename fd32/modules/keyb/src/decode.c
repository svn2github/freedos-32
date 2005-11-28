/* Scancode decoding for FD32's Keyborad Driver
 *
 * Copyright (C) 2003 by Luca Abeni
 * Copyright (C) 2004,2005 by Hanzac Chen
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include <dr-env.h>

#include "key.h"
#include "layout.h"

typedef struct s_key_type
{
	struct {
		BYTE data_length: 3;
		BYTE reserved:    1;
		BYTE lock:        1;
		BYTE numlock_af:  1;
		BYTE capslock_af: 1;
		BYTE s_flag:      1;
	} flags;
	BYTE cmdbits;
	WORD data[4];	/* a variable data */
} s_key_type_t;

const s_key_type_t keymap[128] = {
      /*      Flags,    CMD,        N,   SHFT,   CTRL,     ALT */
	{ {3, 0, 0, 0, 0, 1}, 0,  {     0,      0,      0,      0} },
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x011B, 0x011B, 0x011B, 0x0100} }, /* escape */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0231, 0x0221,      0, 0x7800} }, /* 1! */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0332, 0x0340, 0x0300, 0x7900} }, /* 2@ */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0433, 0x0423,      0, 0x7A00} }, /* 3# */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0534, 0x0524,      0, 0x7B00} }, /* 4$ */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0635, 0x0625,      0, 0x7C00} }, /* 5% */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0736, 0x075E, 0x071E, 0x7D00} }, /* 6^ */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0837, 0x0826,      0, 0x7E00} }, /* 7& */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0938, 0x092A,      0, 0x7F00} }, /* 8* */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0A39, 0x0A28,      0, 0x8000} }, /* 9( */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0B30, 0x0B29,      0, 0x8100} }, /* 0) */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0C2D, 0x0C5F, 0x0C1F, 0x8200} }, /* -_ */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0D3D, 0x0D2B,      0, 0x8300} }, /* =+ */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0E08, 0x0E08, 0x0E7F, 0x0E00} }, /* backspace */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x0F09, 0x0F00, 0x9400, 0xA500} }, /* tab */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x1071, 0x1051, 0x1011, 0x1000} }, /* Q */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x1177, 0x1157, 0x1117, 0x1100} }, /* W */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x1265, 0x1245, 0x1205, 0x1200} }, /* E */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x1372, 0x1352, 0x1312, 0x1300} }, /* R */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x1474, 0x1454, 0x1414, 0x1400} }, /* T */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x1579, 0x1559, 0x1519, 0x1500} }, /* Y */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x1675, 0x1655, 0x1615, 0x1600} }, /* U */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x1769, 0x1749, 0x1709, 0x1700} }, /* I */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x186F, 0x184F, 0x180F, 0x1800} }, /* O */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x1970, 0x1950, 0x1910, 0x1900} }, /* P */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x1A5B, 0x1A7B, 0x1A1B, 0x1A00} }, /* [{ */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x1B5D, 0x1B7D, 0x1B1D, 0x1B00} }, /* ]} */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x1C0D, 0x1C0D, 0x1C0A, 0xA600} }, /* Enter */
	{ {3, 0, 0, 0, 0, 1}, 0,  {     0,      0,      0,      0} }, /* L Ctrl */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x1E61, 0x1E41, 0x1E01, 0x1E00} }, /* A */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x1F73, 0x1F53, 0x1F13, 0x1F00} }, /* S */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x2064, 0x2044, 0x2004, 0x2000} }, /* D */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x2166, 0x2146, 0x2106, 0x2100} }, /* F */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x2267, 0x2247, 0x2207, 0x2200} }, /* G */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x2368, 0x2348, 0x2308, 0x2300} }, /* H */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x246A, 0x244A, 0x240A, 0x2400} }, /* J */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x256B, 0x254B, 0x250B, 0x2500} }, /* K */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x266C, 0x264C, 0x260C, 0x2600} }, /* L */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x273B, 0x273A,      0, 0x2700} }, /* ;: */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x2827, 0x2822,      0,      0} }, /* '" */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x2960, 0x297E,      0,      0} }, /* `~ */
	{ {3, 0, 0, 0, 0, 1}, 0,  {     0,      0,      0,      0} }, /* L shift */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x2B5C, 0x2B7C, 0x2B1C, 0x2600} }, /* |\ */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x2C7A, 0x2C5A, 0x2C1A, 0x2C00} }, /* Z */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x2D78, 0x2D58, 0x2D18, 0x2D00} }, /* X */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x2E63, 0x2E43, 0x2E03, 0x2E00} }, /* C */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x2F76, 0x2F56, 0x2F16, 0x2F00} }, /* V */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x3062, 0x3042, 0x3002, 0x3000} }, /* B */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x316E, 0x314E, 0x310E, 0x3100} }, /* N */
	{ {3, 0, 0, 0, 1, 1}, 0,  {0x326D, 0x324D, 0x320D, 0x3200} }, /* M */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x332C, 0x333C,      0,      0} }, /* ,< */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x342E, 0x343E,      0,      0} }, /* .> */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x352F, 0x353F,      0,      0} }, /* /? */
	{ {3, 0, 0, 0, 0, 1}, 0,  {     0,      0,      0,      0} }, /* R Shift */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x372A, 0x372A, 0x9600, 0x3700} }, /* * */
	{ {3, 0, 0, 0, 0, 1}, 0,  {     0,      0,      0,      0} }, /* L Alt */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x3920, 0x3920, 0x3920, 0x3920} }, /* space */
	{ {3, 0, 0, 0, 0, 1}, 0,  {     0,      0,      0,      0} }, /* caps lock */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x3B00, 0x5400, 0x5E00, 0x6800} }, /* F1 */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x3C00, 0x5500, 0x5F00, 0x6900} }, /* F2 */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x3D00, 0x5600, 0x6000, 0x6A00} }, /* F3 */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x3E00, 0x5700, 0x6100, 0x6B00} }, /* F4 */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x3F00, 0x5800, 0x6200, 0x6C00} }, /* F5 */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x4000, 0x5900, 0x6300, 0x6D00} }, /* F6 */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x4100, 0x5A00, 0x6400, 0x6E00} }, /* F7 */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x4200, 0x5B00, 0x6500, 0x6F00} }, /* F8 */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x4300, 0x5C00, 0x6600, 0x7000} }, /* F9 */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x4400, 0x5D00, 0x6700, 0x7100} }, /* F10 */
	{ {3, 0, 0, 0, 0, 1}, 0,  {     0,      0,      0,      0} }, /* Num Lock */
	{ {3, 0, 0, 0, 0, 1}, 0,  {     0,      0,      0,      0} }, /* Scroll Lock */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x4700, 0x4737, 0x7700, 0x9700} }, /* 7 Home */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x4800, 0x4838, 0x8D00, 0x9800} }, /* 8 UP */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x4900, 0x4939, 0x8400, 0x9900} }, /* 9 PgUp */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x4A2D, 0x4A2D, 0x8E00, 0x4A00} }, /* - */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x4B00, 0x4B34, 0x7300, 0x9B00} }, /* 4 Left */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x0000, 0x4C35, 0x8F00,      0} }, /* 5 */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x4D00, 0x4D36, 0x7400, 0x9D00} }, /* 6 Right */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x4E2B, 0x4E2B,      0, 0x4E00} }, /* + */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x4F00, 0x4F31, 0x7500, 0x9F00} }, /* 1 End */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x5000, 0x5032, 0x9100, 0xA000} }, /* 2 Down */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x5100, 0x5133, 0x7600, 0xA100} }, /* 3 PgDn */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x5200, 0x5230, 0x9200, 0xA200} }, /* 0 Ins */
	{ {3, 0, 0, 1, 0, 1}, 0,  {0x5300, 0x532E, 0x9300, 0xA300} }, /* Del */
	{ {3, 0, 0, 0, 0, 1}, 0,  {     0,      0,      0,      0} },
	{ {3, 0, 0, 0, 0, 1}, 0,  {     0,      0,      0,      0} },
	{ {3, 0, 0, 0, 0, 1}, 0,  {     0,      0,      0,      0} },
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x8500, 0x8700, 0x8900, 0x8B00} }, /* F11 */
	{ {3, 0, 0, 0, 0, 1}, 0,  {0x8600, 0x8800, 0x8A00, 0x8C00} }  /* F12 */
};

WORD keyb_decode(BYTE c, keyb_std_status_t stdst)
{
	int res;

	/* Layout decode first */
	if ((res = keyb_layout_decode(c, stdst)) < 0) {

		/* Set the keymap index value according to the shift flags */
		if (stdst.s.alt) {
			res = 3;
		} else if (stdst.s.ctrl) {
			res = 2;
		} else if (stdst.s.e0_prefix && keymap[c].flags.numlock_af) {
			res = 0;
		} else {
			/*	Letter keys have to be handled according to CAPSLOCK...
				Keypad keys have to be handled according to NUMLOCK...
			*/
			int caps_or_num = (keymap[c].flags.numlock_af && stdst.s.numlk_active) ||
						(keymap[c].flags.capslock_af && stdst.s.capslk_active);
			if (stdst.s.shift)
				res = (1 ^ caps_or_num);
			else /* Extended keys shouldn't shilft */
				res = (0 ^ caps_or_num);
		}

		res = keymap[c].data[res];
	}

	/* return 0 if something like tab has been pressed... */
	return res;
}
