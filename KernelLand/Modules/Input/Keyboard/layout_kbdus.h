
#ifndef _KEYMAP__LAYOUT_KBDUS_H_
#define _KEYMAP__LAYOUT_KBDUS_H_

#include "keymap_int.h"

// - Base (NO PREFIX)
tKeymapLayer	gpKBDUS1 = {
	KEYSYM_SLASH+1,
	{
	 0, 0, 0, 0,
	'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
	'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
	'1', '2', '3', '4', '5', '6', '7', '8', '9', '0',
	'\n', '\x1b', '\b', '\t', ' ', '-', '=', '[', ']', '\\', '#', ';',
	'\'', '`', ',', '.', '/',
//	KEY_CAPSLOCK,
//	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6,
//	KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
//	0, KEY_SCROLLLOCK, KEY_PAUSE, KEY_INS, KEY_HOME, KEY_PGUP, KEY_PGDOWN,
//	KEY_RIGHT, KEY_LEFT, KEY_UP, KEY_DOWN,
//	KEY_NUMLOCK, KEY_KPSLASH, KEY_KPSTAR, KEY_KPMINUS,
//	KEY_KPPLUS, KEY_KPENTER,
//	KEY_KPEND, KEY_KPDOWN, KEY_KPLEFT,
	}
};
tKeymapLayer	gpKBDUS1s = {
	KEYSYM_SLASH+1,
	{
	 0, 0, 0, 0,
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
	'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
	'!', '@', '#', '$', '%', '^', '&', '*', '(', ')',
	'\n', '\x1b', '\b', '\t', ' ', '_', '+', '{', '}', '|', '~', ':',
	'\'', '~', '<', '>', '?',
	}
};

tKeymap	gKeymap_KBDUS = {
	"en-us",
	2,
	{&gpKBDUS1, &gpKBDUS1s}
};

#endif

