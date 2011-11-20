
#ifndef _KBDUS_H
#define _KBDUS_H

// - Base (NO PREFIX)
Uint32	gpKBDUS1[256] = {
	0,
	KEY_ESC, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',	// 0x01 - 0x0e
	'\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	// 0x0f - 0x1c
	KEY_LCTRL, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';','\'',	// 0x1d - 0x28
	'`', KEY_LSHIFT,'\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', KEY_RSHIFT,	// 0x29 - 0x3e
	KEY_KPSTAR,
	KEY_LALT, ' ', KEY_CAPSLOCK,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10,
	KEY_NUMLOCK, KEY_SCROLLLOCK,
	KEY_KPHOME, KEY_KPUP, KEY_KPPGUP, KEY_KPMINUS,
	KEY_KPLEFT, KEY_KP5, KEY_KPRIGHT, KEY_KPPLUS,
	KEY_KPEND, KEY_KPDOWN, KEY_KPPGDN,
	KEY_KPINS, KEY_KPDEL,
	0, 0, 0, KEY_F11, KEY_F12, 0, 0, 0, 0, 0, 0, 0,
/*60*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*70*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
// Shift Key pressed
Uint32	gpKBDUS1s[256] = {
	0,
	KEY_ESC, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',	// 0x01 - 0x0e
	'\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	// 0x0f - 0x1c
	KEY_LCTRL, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':','"',	// 0x1d - 0x28
	'~', KEY_LSHIFT,'|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', KEY_RSHIFT,	// 0x29 - 0x3e
	0
	};
// - 0xE0 Prefixed
Uint32	gpKBDUS2[256] = {
//   	0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
/*00*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0-F
/*10*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_KPENTER, KEY_RCTRL, 0, 0,
/*20*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*30*/	0, 0, 0, 0, 0, KEY_KPSLASH, 0, 0, KEY_RALT, 0, 0, 0, 0, 0, 0, 0,
/*40*/	0, 0, 0, 0, 0, 0, 0, KEY_HOME,
			KEY_UP, KEY_PGUP, 0, KEY_LEFT, 0, KEY_RIGHT, 0, KEY_END,
/*50*/	KEY_DOWN, KEY_PGDOWN, KEY_INS, KEY_DEL, 0, 0, 0, 0,
			0, 0, KEY_WIN, 0, 0, KEY_MENU, 0, 0,
/*60*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*70*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
// - 0xE1 Prefixed
Uint32	gpKBDUS3[256] = {
//   	0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
/*00*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0-F
/*10*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEY_PAUSE, 0, 0,
/*20*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*30*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*40*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*50*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*60*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*70*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


Uint32	*gpKBDUS[6] = { gpKBDUS1, gpKBDUS1s, gpKBDUS2, gpKBDUS2, gpKBDUS3, gpKBDUS3 };

#endif
