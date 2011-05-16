/*
 * Acess2
 *
 * PS/2 Keboard / Mouse Driver
 */
#include <acess.h>
#include <modules.h>

// === IMPORTS ===
extern int	KB_Install(char **Arguments);
extern int	Mouse_Install(char **Arguments);

// === PROTOTYPES ===
 int	PS2_Install(char **Arguments);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0100, Input_PS2KbMouse, PS2_Install, NULL, NULL);
MODULE_DEFINE(0, 0x0100, PS2Keyboard, KB_Install, NULL, NULL);

// === CODE ===
int PS2_Install(char **Arguments)
{
//	KB_Install(Arguments);
//	Mouse_Install(Arguments);
	return MODULE_ERR_OK;
}
