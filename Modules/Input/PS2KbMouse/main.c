/*
 * Acess2
 *
 * PS/2 Keboard / Mouse Driver
 */
#include <acess.h>
#include <modules.h>
#include "common.h"

// === IMPORTS ===

// === PROTOTYPES ===
 int	PS2_Install(char **Arguments);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0100, Input_PS2KbMouse, PS2_Install, NULL, NULL);	// Shuts the makefile up
MODULE_DEFINE(0, 0x0100, PS2Keyboard, KB_Install, NULL, NULL);
MODULE_DEFINE(0, 0x0100, PS2Mouse, PS2Mouse_Install, NULL, NULL);

// === CODE ===
int PS2_Install(char **Arguments)
{
	KBC8042_Init();
	return MODULE_ERR_OK;
}
