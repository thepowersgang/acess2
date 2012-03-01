
#ifndef _KBDUS_H
#define _KBDUS_H

// - Base (NO PREFIX)
Uint32	gp101_to_HID_1[128] = {
	0,
	// First row (0x01 - 0x0e)
	KEYSYM_ESC, KEYSYM_1, KEYSYM_2, KEYSYM_3, KEYSYM_4, KEYSYM_5, KEYSYM_6,
	KEYSYM_7, KEYSYM_8, KEYSYM_9, KEYSYM_0, KEYSYM_MINUS, KEYSYM_EQUALS,
	KEYSYM_BACKSP,
	// Second Row (0x0f - 0x1c)
	KEYSYM_TAB, KEYSYM_q, KEYSYM_w, KEYSYM_e, KEYSYM_r, KEYSYM_t, KEYSYM_y,
	KEYSYM_u, KEYSYM_i, KEYSYM_o, KEYSYM_p, KEYSYM_SQUARE_OPEN, KEYSYM_SQUARE_CLOSE,
	KEYSYM_RETURN,
	// Third Row (0x1d - 0x28)
	KEYSYM_LEFTCTRL, KEYSYM_a, KEYSYM_s, KEYSYM_d, KEYSYM_f, KEYSYM_g, KEYSYM_h,
	KEYSYM_j, KEYSYM_k, KEYSYM_l, KEYSYM_SEMICOLON, KEYSYM_QUOTE,	// 0x1d - 0x28
	// Fourth Row (0x20 - 0x3e)
	KEYSYM_GRAVE_TILDE, KEYSYM_LEFTSHIFT, KEYSYM_BACKSLASH, KEYSYM_z, KEYSYM_x,
	KEYSYM_c, KEYSYM_v, KEYSYM_b, KEYSYM_n, KEYSYM_m, KEYSYM_COMMA, KEYSYM_PERIOD,
	KEYSYM_SLASH, KEYSYM_RIGHTSHIFT,
	// Bottom row (0x3f - 0x42)
	KEYSYM_KPSTAR, KEYSYM_LEFTALT, KEYSYM_SPACE, KEYSYM_CAPS,
	// F Keys (0x43 - 0x4d)
	KEYSYM_F1, KEYSYM_F2, KEYSYM_F3, KEYSYM_F4, KEYSYM_F5,
	KEYSYM_F6, KEYSYM_F7, KEYSYM_F8, KEYSYM_F9, KEYSYM_F10,
	// Keypad
	KEYSYM_NUMLOCK, KEYSYM_SCROLLLOCK,
	KEYSYM_KP7, KEYSYM_KP8,	KEYSYM_KP9, KEYSYM_KPMINUS,
	KEYSYM_KP4, KEYSYM_KP5,	KEYSYM_KP6, KEYSYM_KPPLUS,
	KEYSYM_KP1, KEYSYM_KP2, KEYSYM_KP3,
	KEYSYM_KP0, KEYSYM_KPPERIOD,
	0, 0, 0, KEYSYM_F11, KEYSYM_F12, 0, 0, 0, 0, 0, 0, 0,
/*60*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*70*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
// - 0xE0 Prefixed
Uint32	gp101_to_HID_2[128] = {
//   	0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
/*00*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0-F
/*10*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEYSYM_KPENTER, KEYSYM_RIGHTCTRL, 0, 0,
/*20*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*30*/	0, 0, 0, 0, 0, KEYSYM_KPSLASH, 0, 0, KEYSYM_RIGHTALT, 0, 0, 0, 0, 0, 0, 0,
/*40*/	0, 0, 0, 0, 0, 0, 0, KEYSYM_HOME,
	KEYSYM_UPARROW, KEYSYM_PGUP, 0, KEYSYM_LEFTARROW, 0, KEYSYM_RIGHTARROW, 0, KEYSYM_END,
/*50*/	KEYSYM_DOWNARROW, KEYSYM_PGDN, KEYSYM_INSERT, KEYSYM_DELETE, 0, 0, 0, 0,
	0, 0, 0, KEYSYM_LEFTGUI, KEYSYM_RIGHTGUI, KEYSYM_APPLICATION, 0, 0,
/*60*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*70*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};
// - 0xE1 Prefixed
Uint32	gp101_to_HID_3[128] = {
//   	0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F
/*00*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0-F
/*10*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, KEYSYM_PAUSE, 0, 0,
/*20*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*30*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*40*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*50*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*60*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
/*70*/	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


Uint32	*gp101_to_HID[3] = { gp101_to_HID_1, gp101_to_HID_2, gp101_to_HID_3 };

#endif
