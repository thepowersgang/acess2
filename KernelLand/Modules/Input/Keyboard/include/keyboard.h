/*
 * Acess2 Kernel - Keyboard mulitplexer/translation
 * - By John Hodge (thePowersGang)
 *
 * keyboard.h
 * - Interface header
 */
#ifndef _KEYBOARD__KEYBOARD_H_
#define _KEYBOARD__KEYBAORD_H_

#include <api_drv_keyboard.h>

/**
 * \brief Keyboard instance
 *
 * Used to refer to a keyboard state (current key states, keymap and destination
 * node)
 */
typedef struct sKeyboard	tKeyboard;

/**
 * \brief Create a keyboard instance
 * \param MaxSym	Maximum key symbol that could be passed to Keyboard_HandleKey
 * \param Ident 	Identifier for this keyboard (e.g. "PS2Keyboard")
 * \return Keyboard instance pointer
 * 
 * The \a MaxSym parameter is used to create a bitmap of key states
 */
extern tKeyboard	*Keyboard_CreateInstance(int MaxSym, const char *Ident);
/**
 * \brief De-register and free a keyboard instance
 * \param Instance	Value returned by Keyboard_CreateInstance
 */
extern void	Keyboard_RemoveInstance(tKeyboard *Instance);
/**
 * \brief Handle a key press/release event from the driver
 * \param Instance	Keyboard instance returned by Keyboard_CreateInstance
 * \param HIDKeySym	USB HID Key symbol (KEYSYM_* in Kernel/include/keysym.h), bit 31 denotes release
 *
 * The value in \a HIDKeySym is a USB HID key symbol, but this could come from anywhere.
 * The topmost bit of the 32-bit value is used to denote the key being released, if it is set
 * the key state is cleared and a release event is passed along. Otherwise it is set, 
 * and a refire or a press event is passed (depending on the original key state)
 */
extern void	Keyboard_HandleKey(tKeyboard *Instance, Uint32 HIDKeySym);

#endif

