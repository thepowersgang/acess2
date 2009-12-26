/**
 * \file tpl_drv_keyboard.h
 * \brief Keyboard Driver Interface Definitions
 * \author John Hodge (thePowersGang)
 * 
 * \section dirs VFS Layout
 * Keyboard drivers consist of only a single node, which is a normal file
 * node with a size of zero. All reads and writes to this node are ignored
 * (tVFS_Node.Read and tVFS_Node.Write are NULL)
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
	/**
	 * ioctl(..., int *Rate)
	 * \brief Get/Set Repeat Rate
	 * \param Rate	New repeat rate (pointer)
	 * \return Current/New Repeat rate
	 * 
	 * Gets/Set the repeat rate (actually the time in miliseconds between
	 * repeats) of a held down key.
	 * If the rate is set to zero, repeating will be disabled.
	 */
	KB_IOCTL_REPEATRATE = 4,
	
	/**
	 * ioctl(..., int *Delay)
	 * \brief Get/Set Repeat Delay
	 * \param Delay	New repeat delay (pointer)
	 * \return Current/New repeat delay
	 * 
	 * Gets/Set the time in miliseconds before a key starts repeating
	 * after a key is pressed.
	 * Setting the delay to a negative number will cause the function to
	 * return -1
	 */
	KB_IOCTL_REPEATDELAY,
	
	
	/**
	 * ioctl(..., tKeybardCallback *Callback)
	 * \brief Sets the callback
	 * \note Can be called from kernel mode only
	 * 
	 * Sets the function to be called when a key event occurs (press, release
	 * or repeat). This function pointer must be in kernel mode (although,
	 * kernel->user or kernel->ring3driver abstraction functions can be used)
	 */
	KB_IOCTL_SETCALLBACK
};

/**
 * \brief Callback type for KB_IOCTL_SETCALLBACK
 * \param Key Unicode character code for the pressed key (with bit 31
 *            set if the key is released)
 */
typedef void (*tKeybardCallback)(Uint32 Key);

/**
 * \brief Symbolic key codes
 * 
 * These key codes represent non-pritable characters and are placed above
 * the Unicode character space.
 * If the using driver recieves a key code with the 31st bit set, it means
 * that that key has been released.
 */
enum eTplKeyboard_KeyCodes {
	KEY_ESC = 0x1B,	//!< Escape Character
	
	KEY_NP_MASK = 0x40000000,	//! Mask for non-printable characters
	
	/**
	 * \name Special Keys
	 * \brief These keys are usually used on their own
	 * \{
	 */
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
	/**
	 * \}
	 */
	
	// Modifiers
	/**
	 * \name Modifiers
	 * \brief These keye usually alter the character stream sent to the user
	 * \{
	 */
	KEY_MODIFIERS = 0x60000000,
	KEY_LCTRL, KEY_RCTRL,
	KEY_LALT, KEY_RALT,
	KEY_LSHIFT, KEY_RSHIFT,
	/**
	 * \}
	 */
};


#endif
