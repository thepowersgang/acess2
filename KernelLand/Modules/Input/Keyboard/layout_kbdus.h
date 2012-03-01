/*
 * Acess2 Keyboard Driver
 * - By John Hodge (thePowersGang)
 *
 * layout_kbdus.h
 * - US Keyboard Layout
 *
 * TODO: Support Num-Lock
 */
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
	0,	// Capslock
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	// F1 -> F12
	0, 0, 0, 0, 0, 0, 0,	// ?, ScrollLock, Pause, Insert, Home, PgUp, PgDn
	0, 0, 0, 0,	// Right, Left, Up, Down
	0, '/', '*', '-', '+', '\n',	// NumLock, Keypad /, *, -, +, Enter
//	KEYSYM_KPEND, KEYSYM_KPDOWN, KEYSYM_KPPGDN,
//	KEYSYM_KPLEFT, KEYSYM_KP5, KEY_KPRIGHT,
//	KEYSYM_KPHOME, KEYSYM_KPUP, KEYSYM_KPPGUP,
//	KEYSYM_PKINS, KEYSYM_KPDEL
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
	2, {&gpKBDUS1, &gpKBDUS1s}
};

#endif

