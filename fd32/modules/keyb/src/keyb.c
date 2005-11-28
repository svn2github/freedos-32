/* i8042 and keyboard handling for FD32's Keyborad Driver
 *
 * Copyright (C) 2003,2004 by Luca Abeni
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
#include "queues.h"

typedef union psctrl_status
{
	BYTE data;
	struct
	{
		BYTE outbuf_full       :1;
		BYTE inbuf_full        :1;
		BYTE self_test         :1;
		BYTE command           :1;
		BYTE unlocked          :1;
		BYTE mouse_outbuf_full :1;
		BYTE general_timeout   :1;
		BYTE parity_error      :1;
	} s;
} psctrl_status_t;

/*
 * i8042 low-level functions
 */
BYTE keyb_get_status(void)
{
	return fd32_inb(KEYB_STATUS_PORT);
}

BYTE keyb_get_data(void)
{
	return fd32_inb(KEYB_DATA_PORT);
}

void keyb_send_command(BYTE val)
{
	fd32_outb(val, KEYB_STATUS_PORT);
}

static int keyb_wait_write(void)
{
	DWORD i;
	psctrl_status_t st;
	
	for (i = 0, st.data = keyb_get_status(); st.s.inbuf_full && i < KEYB_CTL_TIMEOUT; i++, st.data = keyb_get_status())
		delay(50);
	if (i < KEYB_CTL_TIMEOUT)
		return 0;
	else
		return -1;
}

static int keyb_wait_read(void)
{
	DWORD i;
	BYTE t;
	psctrl_status_t st;

	for (i = 0, st.data = keyb_get_status(); st.s.outbuf_full && i < KEYB_CTL_TIMEOUT; i++, st.data = keyb_get_status())
		t = fd32_inb(KEYB_DATA_PORT);
	if (i < KEYB_CTL_TIMEOUT)
		return 0;
	else
		return -1;
}

/*
 * Function to write in the keyboard register; it waits for the port
 * to be free, then write the data
 */
int keyb_write(BYTE data)
{
	if (keyb_wait_write() != -1) {
		fd32_outb(KEYB_DATA_PORT, data);
		return 0;
	} else {
		return -1;
	}
}

/*
 * Keyboard handling
 */

WORD keyb_get_shift_flags(void)
{
	return bios_da.keyb_flag;
}

int keyb_set_shift_flags(WORD f)
{
	bios_da.keyb_flag = f;
	return 0;
}

typedef struct keyb_command {
	BYTE cmd;
	BYTE datalen;
	BYTE *data;
} keyb_command_t;

static keyb_std_status_t stdst;
static WORD ack_left;
static BYTE ack_cmd;
static BYTE led_state;
static keyb_command_t cmd_table[] = {
	{KEYB_CMD_SET_LEDS, 1, &led_state} /* After the ack, the led state must be sent! */
};
#define CMD_NUM (sizeof(cmd_table)/sizeof(keyb_command_t))

int keyb_send_cmd(BYTE cmd)
{
	if (ack_left == 0)
		if (keyb_write(cmd) != -1) {
			ack_left = 2;   /* Always need 2 ACKs */
			ack_cmd = cmd;	/* Record the ack information */
			return 0;
		}

	fd32_message("[KEYB] control command sending error!\n");
	return -1;
}

static void keyb_ack_handler(void)
{
	DWORD i, j;

	switch (ack_left) {
		case 2:
			/* Send the command parameter */
			for (i = 0; i < CMD_NUM; i++)
				if (ack_cmd == cmd_table[i].cmd)
					for (j = 0; j < cmd_table[i].datalen; j++)
						keyb_write(cmd_table[i].data[j]);
			ack_left--;
			break;
		case 1:
			/* NOTE: Shall we need: keyb_write(KBD_CMD_ENABLE); */
			ack_left--;
			break;
		default:
			/* This is impossible!!! */
			fd32_message("[KEYB] Keyboard command acknowledgement CRITICAL error (current ack: %u)\n", ack_left);
			fd32_error("Keyboard driver: inconsistent internal status!\n");
			break;
	}
}

void keyb_state_reset(void)
{
	stdst.data = bios_da.keyb_flag;
}

/*
 * Function mostly handle the LEDs
 */
int preprocess(BYTE code)
{
	int ret = 1;
	int e0_keycode_1 = 0;
	static int e0_keycode = 0;

	/* Add the extended key prefix */
	if (e0_keycode) {
		/* Extended keycode */
		e0_keycode_1 = 0x80;
		e0_keycode = 0;
	}

	switch(code) {
		case 0xE0:
			e0_keycode = 1;
			break;
		case 0xFA:
			keyb_ack_handler();
			break;
		/* Extended KEY, manage the instert status */
		case MK_INSERT:
			bios_da.keyb_flag ^= ACT_INSERT;
			ret = 0;
			break;
		/* Normal FUNCTION KEY pressed ... */
		case MK_CTRL:
			stdst.s.ctrl = 1;
			bios_da.keyb_flag |= FLAG_CTRL;
			if (!e0_keycode_1) {
				stdst.s.lctrl = 1;
				bios_da.keyb_flag |= FLAG_LCTRL;
			} else { /* Extended RCTRL */
				stdst.s.rctrl = 1;
				bios_da.keyb_mode |= MODE_RCTRL;
			}
			break;
		case MK_LSHIFT:
			stdst.s.shift = stdst.s.lshift = 1;
			bios_da.keyb_flag |= FLAG_LSHIFT;
			/* Numlock and shift conditions automatically cared in extended key */
			break;
		case MK_RSHIFT:
			stdst.s.shift = stdst.s.rshift = 1;
			bios_da.keyb_flag |= FLAG_RSHIFT;
			break;
		case MK_ALT:
			stdst.s.alt = 1;
			bios_da.keyb_flag |= FLAG_ALT;
			if (!e0_keycode_1) {
				stdst.s.lalt = 1;
				bios_da.keyb_flag |= FLAG_LALT;
			} else { /* Extended RALT */
				stdst.s.ralt = 1;
				bios_da.keyb_mode |= MODE_RALT;
			}
			break;
		case MK_CAPS:
			stdst.s.capslk_active ^= 1;
			bios_da.keyb_flag |= FLAG_CAPS;
			bios_da.keyb_flag ^= ACT_CAPSLK;
			bios_da.keyb_led ^= LED_CAPSLK;
			led_state = bios_da.keyb_led&LEGS_MASK;
			keyb_send_cmd(KEYB_CMD_SET_LEDS);
			break;
		case MK_NUMLK:
			stdst.s.numlk_active ^= 1;
			bios_da.keyb_flag |= FLAG_NUMLK;
			bios_da.keyb_flag ^= ACT_NUMLK;
			bios_da.keyb_led ^= LED_NUMLK;
			led_state = bios_da.keyb_led&LEGS_MASK;
			keyb_send_cmd(KEYB_CMD_SET_LEDS);
			break;
		case MK_SCRLK:
			stdst.s.scrlk_active ^= 1;
			bios_da.keyb_flag |= FLAG_SCRLK;
			bios_da.keyb_flag ^= ACT_SCRLK;
			bios_da.keyb_led ^= LED_SCRLK;
			led_state = bios_da.keyb_led&LEGS_MASK;
			keyb_send_cmd(KEYB_CMD_SET_LEDS);
			break;
		case MK_SYSRQ:
			bios_da.keyb_flag |= FLAG_SYSRQ;
			fd32_message("SYSRQ pressed ...\n");
			break;
		/* Normal FUNCTION KEY released ... */
		case BREAK|MK_CTRL:
			if (!e0_keycode_1) {
				stdst.s.lctrl = 0;
				bios_da.keyb_flag &= ~FLAG_LCTRL;
			} else { /* Extended RCTRL */
				stdst.s.rctrl = 0;
				bios_da.keyb_mode &= ~MODE_RCTRL;
			}
			stdst.s.ctrl = stdst.s.lctrl|stdst.s.rctrl;
			bios_da.keyb_flag &= ~(FLAG_CTRL*~stdst.s.ctrl);
			break;
		case BREAK|MK_LSHIFT:
			stdst.s.lshift = 0;
			stdst.s.shift = stdst.s.lshift|stdst.s.rshift;
			bios_da.keyb_flag &= ~FLAG_LSHIFT;
			/* Numlock and shift conditions automatically cared in extended key */
			break;
		case BREAK|MK_RSHIFT:
			stdst.s.rshift = 0;
			stdst.s.shift = stdst.s.lshift|stdst.s.rshift;
			bios_da.keyb_flag &= ~FLAG_RSHIFT;
			break;
		case BREAK|MK_ALT:
			if (!e0_keycode_1) {
				stdst.s.lalt = 0;
				bios_da.keyb_flag &= ~FLAG_LALT;
			} else { /* Extended RALT */
				stdst.s.ralt = 0;
				bios_da.keyb_mode &= ~MODE_RALT;
			}
			stdst.s.alt = stdst.s.lalt|stdst.s.ralt;
			bios_da.keyb_flag &= ~(FLAG_ALT*~stdst.s.alt);
			break;
		case BREAK|MK_CAPS:
			bios_da.keyb_flag &= ~FLAG_CAPS;
			break;
		case BREAK|MK_NUMLK:
			bios_da.keyb_flag &= ~FLAG_NUMLK;
			break;
		case BREAK|MK_SCRLK:
			bios_da.keyb_flag &= ~FLAG_SCRLK;
			break;
		case BREAK|MK_SYSRQ:
			bios_da.keyb_flag &= ~FLAG_SYSRQ;
			fd32_message("SYSRQ released ...\n");
			break;
		default:
			if (!(BREAK&code))
				ret = 0;
			break;
	}

	if (!ret)
		rawqueue_put(code|e0_keycode_1);

	return ret;
}

/* Interrupts are enabled, here... */
void postprocess(void)
{
	WORD decoded = 0;
	BYTE code = rawqueue_read();

	while (code != 0) {
		if (code&0x80) { /* Extended code */
			stdst.s.e0_prefix = 1;
			code &= 0x7F;
		}
		/* fire the hook if exists */
		keyb_fire_hook(code, stdst.s.ctrl, stdst.s.alt);

		/* Handle the basic key */
		decoded = keyb_decode(code, stdst);
		/* Reset the e0_prefix flag */
		stdst.s.e0_prefix = 0;
		/* Decode it... And put the scancode and charcode into the keyqueue */
		if (decoded == 0) {
			fd32_log_printf("Strange key: %d (0x%x)\n", code, code);
			/* Skip the keystroke */
			keyqueue_put(0x0000);
			keyqueue_get();
		} else {
			keyqueue_put(decoded);
			decoded = 0;
		}
		/* Retrieve raw code again */
		code = rawqueue_read();
	}
}
