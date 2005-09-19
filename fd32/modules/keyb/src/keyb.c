/* Keyborad Driver for FD32
 * by Luca Abeni and Hanzac Chen
 *
 * 2004 - 2005
 * This is free software; see GPL.txt
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

static keyb_std_status_t stdst;
static int ack_expected = 0;
static WORD ecode = 0;

BYTE keyb_get_status(void)
{
	return fd32_inb(KEYB_STATUS_PORT);
}

BYTE keyb_get_data(void)
{
	return fd32_inb(KEYB_DATA_PORT);
}

/*
 * Function to write in the keyboard register; it waits for the port
 * to be free, then write the command
 */
int keyb_write(BYTE cmd)
{
	DWORD i;
	BYTE t;
	psctrl_status_t st;

	for (i = 0, st.data = keyb_get_status(); (st.s.outbuf_full|st.s.inbuf_full) && i < 0x1000000; i++, st.data = keyb_get_status())
		if (st.s.outbuf_full)
			t = fd32_inb(0x60);
	if (i < 0x1000000) {
		fd32_outb(KEYB_DATA_PORT, cmd);
		return 1;
	} else {
		return -1;
	}
}

static void set_leds(void)
{
	if (ack_expected != 0) {
		/* Uhmm.... Why is this != 0? */
		fd32_error("Set Leds with ack_expected != 0!!!\n");
		/* The best thing to do here is to reset the keyboard... */
		return;
	}
	ack_expected = 1;
	keyb_write(KEYB_CMD_SET_LEDS);
}

WORD keyb_get_shift_flags(void)
{
	return bios_da.keyb_flag;
}

void keyb_set_shift_flags(WORD f)
{
	bios_da.keyb_flag = f;
	set_leds();
}

static void handle_ack(void)
{
	if (ack_expected == 0) {
		/* What happened? An ack arrived but we are not waiting for it... */
		fd32_error("WARNING: Unexpected Ack!!!\n");
	} else if (ack_expected == 1) {
		/* We sent a 0xED command... After the ack, the led mask must be sent! */
		keyb_write((bios_da.keyb_flag&LEGS_MASK)>>4);
		ack_expected = 2;
	} else if (ack_expected == 2) {
		/* Uhmmm... We don't need this!
		keyb_write(KBD_CMD_ENABLE); */
		ack_expected = 0;
	} else {
		/* This is impossible!!! */
		fd32_message("[KEYB] CRITICAL: ack_expected = %d\n", ack_expected);
		fd32_error("Keyboard driver: inconsistent internal status!\n");
	}
}

/*
 * Function mostly handle the LEDs
 */
int preprocess(BYTE code)
{
	switch(code) {
		case 0xE0:
			return 0;
		case 0xFA:
			handle_ack();
			break;
		/* FUNCTION KEY pressed ... */
		case MK_LCTRL:
			stdst.s.ctrl = stdst.s.lctrl = 1;
			bios_da.keyb_flag |= FLAG_LCTRL|FLAG_CTRL;
			break;
		case MK_LSHIFT:
			stdst.s.shift = stdst.s.lshift = 1;
			bios_da.keyb_flag |= FLAG_LSHIFT;
			break;
		case MK_RSHIFT:
			stdst.s.shift = stdst.s.rshift = 1;
			bios_da.keyb_flag |= FLAG_RSHIFT;
			break;
		case MK_LALT:
			stdst.s.alt = stdst.s.lalt = 1;
			bios_da.keyb_flag |= FLAG_LALT|FLAG_ALT;
			break;
		case MK_CAPS:
			stdst.s.capslk_active ^= 1;
			bios_da.keyb_flag |= FLAG_CAPS;
			bios_da.keyb_flag ^= ACT_CAPSLK;
			bios_da.keyb_led ^= LED_CAPSLK;
			set_leds();
			break;
		case MK_NUMLK:
			stdst.s.numlk_active ^= 1;
			bios_da.keyb_flag |= FLAG_NUMLK;
			bios_da.keyb_flag ^= ACT_NUMLK;
			bios_da.keyb_led ^= LED_NUMLK;
			set_leds();
			break;
		case MK_SCRLK:
			stdst.s.scrlk_active ^= 1;
			bios_da.keyb_flag |= FLAG_SCRLK;
			bios_da.keyb_flag ^= ACT_SCRLK;
			bios_da.keyb_led ^= LED_SCRLK;
			set_leds();
			break;
		case MK_SYSRQ:
			bios_da.keyb_flag |= FLAG_SYSRQ;
			fd32_message("SYSRQ pressed ...\n");
			break;

		/* FUNCTION KEY released ... */
		case BREAK|MK_LCTRL:
			stdst.s.lctrl = 0;
			stdst.s.ctrl = stdst.s.lctrl|stdst.s.rctrl;
			bios_da.keyb_flag &= ~FLAG_LCTRL;
			bios_da.keyb_flag &= ~(FLAG_CTRL*~stdst.s.ctrl);
			break;
		case BREAK|MK_LSHIFT:
			stdst.s.lshift = 0;
			stdst.s.shift = stdst.s.lshift|stdst.s.rshift;
			bios_da.keyb_flag &= ~FLAG_LSHIFT;
			break;
		case BREAK|MK_RSHIFT:
			stdst.s.rshift = 0;
			stdst.s.shift = stdst.s.lshift|stdst.s.rshift;
			bios_da.keyb_flag &= ~FLAG_RSHIFT;
			break;
		case BREAK|MK_LALT:
			stdst.s.lalt = 0;
			stdst.s.alt = stdst.s.lalt|stdst.s.ralt;
			bios_da.keyb_flag &= ~FLAG_LALT;
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
			return 0;
	}

	return 1;
}

/* Interrupts are enabled, here... */
void postprocess(void)
{
	WORD decoded = 0;
	BYTE code = rawqueue_get();

	while (code != 0) {
		if (stdst.s.e0_prefix) {
			/* Handle the extended key */
			int done = 1;
			/* Prepare the extended key */
			ecode = 0xE000|code;
			switch(ecode) {
				/* FUNCTION KEY pressed ... */
				case MK_RCTRL:
					stdst.s.rctrl = 1;
					stdst.s.ctrl = 1;
					bios_da.keyb_mode |= MODE_RCTRL;
					bios_da.keyb_flag |= FLAG_CTRL;
					break;
				case MK_RALT:
					stdst.s.ralt = 1;
					stdst.s.alt = 1;
					bios_da.keyb_mode |= MODE_RALT;
					bios_da.keyb_flag |= FLAG_ALT;
					break;
				/* FUNCTION KEY released ... */
				case BREAK|MK_RCTRL:
					stdst.s.rctrl = 0;
					stdst.s.ctrl = stdst.s.lctrl|stdst.s.rctrl;
					bios_da.keyb_mode &= ~MODE_RCTRL;
					bios_da.keyb_flag &= ~(FLAG_CTRL*~stdst.s.ctrl);
					break;
				case BREAK|MK_RALT:
					stdst.s.ralt = 0;
					stdst.s.alt = stdst.s.lalt|stdst.s.ralt;
					bios_da.keyb_mode &= ~MODE_RALT;
					bios_da.keyb_flag &= ~(FLAG_ALT*~stdst.s.alt);
					break;
		
				/* Extended KEY, manage the instert status */
				case MK_INSERT:
					bios_da.keyb_flag ^= ACT_INSERT;
					done = 0;
					break;
				default:
					if (!(BREAK&ecode))
						done = 0;
					break;
			}
			/* Determine if need the extended key */
			if (!done) {
				/* fire the hook if exists */
				keyb_fire_hook(code, stdst.s.ctrl, stdst.s.alt);
				decoded = keyb_decode(code, stdst, bios_da.keyb_led);
				keyqueue_put(decoded);
			}
			stdst.s.e0_prefix = 0;
		} else if (code == 0xE0) {
			stdst.s.e0_prefix = 1;
		} else if (!(BREAK&code)) {
			/* fire the hook if exists */
			keyb_fire_hook(code, stdst.s.ctrl, stdst.s.alt);
			/* Handle the basic key */
			decoded = keyb_decode(code, stdst, bios_da.keyb_led);
			/* Decode it... And put the scancode and charcode into the keyqueue */
			if (decoded == 0) {
				fd32_log_printf("Strange key: %d (0x%x)\n", code, code);
			} else {
				keyqueue_put(decoded);
			}
		}
		/* Retrieve raw code again */
		code = rawqueue_get();
	}
}
