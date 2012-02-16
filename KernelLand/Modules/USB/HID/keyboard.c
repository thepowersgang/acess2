/*
 * Acess2 USB Stack HID Driver
 * - By John Hodge (thePowersGang)
 *
 * keyboard.c
 * - Keyboard translation
 */
#define DEBUG	0
#include <fs_devfs.h>

// === GLOBALS ===
tDevFS_Driver	gHID_DevFS_Keyboard = {
	.Name = "USBKeyboard",
};
