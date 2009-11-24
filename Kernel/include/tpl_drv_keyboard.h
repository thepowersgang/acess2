/**
 * \file tpl_drv_keyboard.h
 * \brief Keyboard Driver Interface Definitions
*/
#ifndef _TPL_KEYBOARD_H
#define _TPL_KEYBOARD_H

#include <tpl_drv_common.h>

/**
 * \enum eTplKeyboard_IOCtl
 * \brief Common Keyboard IOCtl Calls
 * \extends eTplDrv_IOCtl
 */
enum eTplKeyboard_IOCtl {
	//! \brief Get/Set Repeat Rate - (int Rate)
	KB_IOCTL_REPEATRATE = 4,
	//! \brief Get/Set Repeat Delay - (int Delay)
	KB_IOCTL_REPEATDELAY,
	//! \brief Sets the callback
	KB_IOCTL_SETCALLBACK
};

/**
 * \brief Callback type for KB_IOCTL_SETCALLBACK
 */
typedef void (*tKeybardCallback)(Uint32 Key);

/**
 * \brief Symbolic key codes
 */
enum eTplKeyboard_KeyCodes {
	KEY_ESC = 0x1B,
	
	KEY_NP_MASK = 0x40000000,	//End of ASCII Range
	
	KEY_CAPSLOCK,
	KEY_UP, KEY_DOWN, KEY_LEFT, KEY_RIGHT,
	KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, 
	KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12,
	KEY_NUMLOCK, KEY_SCROLLLOCK,
	KEY_HOME, KEY_END, KEY_INS, KEY_DEL,
	KEY_PAUSE, KEY_BREAK,
	KEY_PGUP, KEY_PGDOWN,
	KEY_KPENTER, KEY_KPSLASH, KEY_KPMINUS, KEY_KPPLUS, KEY_KPSTAR,
	KEY_KPHOME, KEY_KPUP, KEY_KPPGUP, KEY_KPLEFT, KEY_KP5, KEY_KPRIGHT,
	KEY_KPEND, KEY_KPDOWN, KEY_KPPGDN, KEY_KPINS, KEY_KPDEL,
	KEY_WIN, KEY_MENU,
	
	// Modifiers
	KEY_MODIFIERS = 0x60000000,
	KEY_LCTRL, KEY_RCTRL,
	KEY_LALT, KEY_RALT,
	KEY_LSHIFT, KEY_RSHIFT,
	
	KEY_KEYUP = 0xFF
};


#endif
