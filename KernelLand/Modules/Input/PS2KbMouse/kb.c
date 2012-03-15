/*
 * Acess2
 * PS2 Keyboard Driver
 */
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <api_drv_common.h>
#include <api_drv_keyboard.h>
#include <Input/Keyboard/include/keyboard.h>
#include "kb_transtab.h"

// === CONSTANTS ===
#define	KB_BUFFER_SIZE	1024
#define	USE_KERNEL_MAGIC	1

// === PROTOTYPES ===
 int	KB_Install(char **Arguments);
void	KB_HandleScancode(Uint8 scancode);
void	KB_UpdateLEDs(void);
 int	KB_IOCtl(tVFS_Node *Node, int Id, void *Data);

// === GLOBALS ===
 int	giPS2Kb_Layer;
 int	gbPS2Kb_KeyUp;
tKeyboard	*gPS2Kb_Info;

// === CODE ===
/**
 * \brief Install the keyboard driver
 */
int KB_Install(char **Arguments)
{
	gPS2Kb_Info = Keyboard_CreateInstance(KEYSYM_RIGHTGUI, "PS2Keyboard");
	return MODULE_ERR_OK;
}

/**
 * \brief Called on a keyboard IRQ
 * \param IRQNum	IRQ number (unused)
 */
void KB_HandleScancode(Uint8 scancode)
{
	Uint32	hidcode;

	// Ignore ACKs
	if(scancode == 0xFA) return;

	// Layer 1
	if(scancode == 0xE0) {
		giPS2Kb_Layer = 1;
		return;
	}
	// Layer 2
	if(scancode == 0xE1) {
		giPS2Kb_Layer = 2;
		return;
	}

	#if KB_ALT_SCANCODES
	if(scancode == 0xF0)
	{
		gbPS2Kb_KeyUp = 1;
		return;
	}
	#else
	if(scancode & 0x80)
	{
		scancode &= 0x7F;
		gbPS2Kb_KeyUp = 1;
	}
	#endif

	hidcode = gp101_to_HID[giPS2Kb_Layer][scancode];
	if( hidcode == 0)
	{
		Log_Warning("PS2Kb", "Unknown scancode %i:0x%x %s", giPS2Kb_Layer, scancode,
			gbPS2Kb_KeyUp ? "release" : "press"
			);
	}
	else if( hidcode == -1 )
	{
		// Ignored (Fake shift)
	}
	else
	{
		if( gbPS2Kb_KeyUp )
			Keyboard_HandleKey( gPS2Kb_Info, (1 << 31) | hidcode );
		else
			Keyboard_HandleKey( gPS2Kb_Info, (0 << 31) | hidcode );
	}
	
	giPS2Kb_Layer = 0;
	gbPS2Kb_KeyUp = 0;
}

/**
 * \fn void KB_UpdateLEDs(void)
 * \brief Updates the status of the keyboard LEDs
 */
void KB_UpdateLEDs(void)
{
//	Uint8	leds;

//	leds = (gbKB_CapsState ? 4 : 0);

	// TODO: Update LEDS
	Log_Warning("Keyboard", "TODO: Update LEDs");
}

