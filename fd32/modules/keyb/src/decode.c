#include <dr-env.h>

#include "key.h"

const char unshifted_keymap[128] = {
0, 27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
'-', '=', 8, '\t', 
'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
13, 0,
'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
0, '\\', 
'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
0, '*', 0, ' ', 0, '\t', 
0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, '7', '8', '9', '-', '4', '5', '6', '+', '1',
'2', '3', '0', '.', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const char shifted_keymap[128] = {
0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
'_', '+', 8, ' ', 
'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
'\n', 0,
'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
0, '|', 
'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
0, '*', 0, ' ', 0, 0, 
0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0/*home*/, 0/*up*/, 0/*pgup*/, 0, 0/*left*/, 0, 0/*right*/, 0, 0/*end*/,
0, 0/*pgdown*/, 0/*ins*/, 0/*del*/, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* I invented this... Dunno how much correct it is */
int keypadscancode(BYTE c)
{
  return ((c >= 71) && (c <= 83));
}

int letterscancode(BYTE c)
{
  return ((unshifted_keymap[c] >= 'a') && (unshifted_keymap[c] <= 'z'));
}

BYTE decode(BYTE c, int flags, int lock)
{
  if (c >= 128) {
    fd32_error("Scancode > 128\n");
    return 0;
  }

  /* Keypad keys have to be handled according to NUMLOCK... */
  if (keypadscancode(c)) {
    if (lock & LED_NUMLK) {
      return shifted_keymap[c];
    }
    return unshifted_keymap[c];
  }

  if (flags & SHIFT_FLAG) {
    return shifted_keymap[c];
  }
  
  if (lock & LED_CAPS) {
    if (letterscancode(c)) {
      return shifted_keymap[c];
    }
    return unshifted_keymap[c];
  }
  if (flags == 0) {
    return unshifted_keymap[c];
  }
  /* If not, something like tab has been pressed... */
  return 0;
}
