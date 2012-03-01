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

typedef struct sKeyboard	tKeyboard;

extern tKeyboard	*Keyboard_CreateInstance(int MaxSym, const char *Ident);
extern void	Keyboard_RemoveInstance(tKeyboard *Instance);
extern void	Keyboard_HandleKey(tKeyboard *Source, Uint32 HIDKeySym);

#endif

