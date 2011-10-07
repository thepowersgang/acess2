/*
 * Acess2
 *
 * PS/2 Keboard / Mouse Driver
 */
#include <acess.h>
#include <modules.h>
#include "common.h"

// === IMPORTS ===
// TODO: Allow runtime/compile-time switching
//       Maybe PCI will have it?
// Integrator-CP
#if 0
#define KEYBOARD_IRQ	3
#define KEYBOARD_BASE	0x18000000
#define MOUSE_IRQ	4
#define MOUSE_BASE	0x19000000
#endif
// Realview
#if 1
#define KEYBOARD_IRQ	20
#define KEYBOARD_BASE	0x10006000
#define MOUSE_IRQ	21
#define MOUSE_BASE	0x10007000
#endif

// === PROTOTYPES ===
 int	PS2_Install(char **Arguments);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0100, Input_PS2KbMouse, PS2_Install, NULL, NULL);	// Shuts the makefile up
MODULE_DEFINE(0, 0x0100, PS2Keyboard, KB_Install, NULL, "Input_PS2KbMouse", NULL);
MODULE_DEFINE(0, 0x0100, PS2Mouse, PS2Mouse_Install, NULL, "Input_PS2KbMouse", NULL);

// === CODE ===
int PS2_Install(char **Arguments)
{
	#if ARCHDIR_is_x86 || ARCHDIR_is_x86_64
	KBC8042_Init();
	gpMouse_EnableFcn = KBC8042_EnableMouse;
	#elif ARCHDIR_is_armv7
	PL050_Init(KEYBOARD_IRQ, KEYBOARD_IRQ, MOUSE_BASE, MOUSE_IRQ);
	gpMouse_EnableFcn = PL050_EnableMouse;
	#endif

	return MODULE_ERR_OK;
}
