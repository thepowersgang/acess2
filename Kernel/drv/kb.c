/*
 * Acess2
 * PS2 Keyboard Driver
 */
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <tpl_drv_common.h>
#include <tpl_drv_keyboard.h>
#include "kb_kbdus.h"

// === CONSTANTS ===
#define	KB_BUFFER_SIZE	1024
#define	USE_KERNEL_MAGIC	1

// === IMPORTS ===
void	Threads_Dump();

// === PROTOTYPES ===
 int	KB_Install(char **Arguments);
void	KB_IRQHandler();
void	KB_AddBuffer(char ch);
Uint64	KB_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Dest);
void	KB_UpdateLEDs();
 int	KB_IOCtl(tVFS_Node *Node, int Id, void *Data);

// === GLOBALS ===
MODULE_DEFINE(0, 0x0100, PS2Keyboard, KB_Install, NULL, NULL);
tDevFS_Driver	gKB_DevInfo = {
	NULL, "PS2Keyboard",
	{
	.NumACLs = 0,
	.Size = 0,
	//.Read = KB_Read,
	.IOCtl = KB_IOCtl
	}
};
tKeybardCallback	gKB_Callback = NULL;
Uint32	**gpKB_Map = gpKBDUS;
Uint8	gbaKB_States[3][256];
 int	gbKB_ShiftState = 0;
 int	gbKB_CapsState = 0;
 int	gbKB_KeyUp = 0;
 int	giKB_KeyLayer = 0;
#if USE_KERNEL_MAGIC
 int	gbKB_MagicState = 0;
#endif
//Uint64	giKB_ReadBase = 0;
//Uint32	gaKB_Buffer[KB_BUFFER_SIZE];	//!< Keyboard Ring Buffer
//volatile int	giKB_InsertPoint = 0;	//!< Writing location marker
//volatile int	giKB_ReadPoint = 0;	//!< Reading location marker
//volatile int	giKB_InUse = 0;		//!< Lock marker

// === CODE ===
/**
 * \fn int KB_Install(char **Arguments)
 */
int KB_Install(char **Arguments)
{
	Uint8	temp;
	
	// Attempt to get around a strange bug in Bochs/Qemu by toggling
	// the controller on and off
	temp = inb(0x61);
	outb(0x61, temp | 0x80);
	outb(0x61, temp & 0x7F);
	inb(0x60);	// Clear keyboard buffer
	
	IRQ_AddHandler(1, KB_IRQHandler);
	DevFS_AddDevice( &gKB_DevInfo );
	//Log("KB_Install: Installed");
	return MODULE_ERR_OK;
}

/**
 * \fn void KB_IRQHandler()
 * \brief Called on a keyboard IRQ
 */
void KB_IRQHandler()
{
	Uint8	scancode;
	Uint32	ch;
	// int	keyNum;

	// Check port 0x64 to tell if this is from the aux port
	if( inb(0x64) & 0x20 )	return;

	scancode = inb(0x60); // Read from the keyboard's data buffer
	//Log_Debug("Keyboard", "scancode = %02x", scancode);

	// Ignore ACKs
	if(scancode == 0xFA) {
		// Oh man! This is anarchic (I'm leaving it here to represent
		// the mess that Acess once was)
		//kb_lastChar = KB_ACK;
		return;
	}

	// Layer +1
	if(scancode == 0xE0) {
		giKB_KeyLayer = 1;
		return;
	}
	// Layer +2
	if(scancode == 0xE1) {
		giKB_KeyLayer = 2;
		return;
	}

	#if KB_ALT_SCANCODES
	if(scancode == 0xF0)
	{
		gbKB_KeyUp = 1;
		return;
	}
	#else
	if(scancode & 0x80)
	{
		scancode &= 0x7F;
		gbKB_KeyUp = 1;
	}
	#endif

	// Translate
	ch = gpKB_Map[giKB_KeyLayer][scancode];
	//keyNum = giKB_KeyLayer * 256 + scancode;
	// Check for unknown key
	if(!ch && !gbKB_KeyUp)
		Log_Warning("Keyboard", "UNK %i %x", giKB_KeyLayer, scancode);

	// Key Up?
	if (gbKB_KeyUp)
	{
		gbKB_KeyUp = 0;
		gbaKB_States[giKB_KeyLayer][scancode] = 0;	// Unset key state flag

		#if USE_KERNEL_MAGIC
		if(ch == KEY_LCTRL)	gbKB_MagicState &= ~1;
		if(ch == KEY_LALT)	gbKB_MagicState &= ~2;
		#endif

		if(ch == KEY_LSHIFT)	gbKB_ShiftState &= ~1;
		if(ch == KEY_RSHIFT)	gbKB_ShiftState &= ~2;

		// Call callback
		if(ch != 0 && gKB_Callback)
			gKB_Callback( ch & 0x80000000 );

		// Reset Layer
		giKB_KeyLayer = 0;
		return;
	}

	// Set the bit relating to the key
	gbaKB_States[giKB_KeyLayer][scancode] = 1;
	// Set shift key bits
	if(ch == KEY_LSHIFT)	gbKB_ShiftState |= 1;
	if(ch == KEY_RSHIFT)	gbKB_ShiftState |= 2;

	// Check for Caps Lock
	if(ch == KEY_CAPSLOCK) {
		gbKB_CapsState = !gbKB_CapsState;
		KB_UpdateLEDs();
	}

	// Reset Layer
	giKB_KeyLayer = 0;

	// Ignore Non-Printable Characters
	if(ch == 0)		return;

	// --- Check for Kernel Magic Combos
	#if USE_KERNEL_MAGIC
	if(ch == KEY_LCTRL) {
		gbKB_MagicState |= 1;
		//Log_Log("Keyboard", "Kernel Magic LCTRL Down\n");
	}
	if(ch == KEY_LALT) {
		gbKB_MagicState |= 2;
		//Log_Log("Keyboard", "Kernel Magic LALT Down\n");
	}
	if(gbKB_MagicState == 3)
	{
		switch(ch)
		{
		case 'd':	__asm__ __volatile__ ("xchg %bx, %bx");	break;
		case 'p':	Threads_Dump();	break;
		}
	}
	#endif

	// Is shift pressed
	// - Darn ugly hacks !(!x) means (bool)x
	if( !(!gbKB_ShiftState) ^ gbKB_CapsState)
	{
		switch(ch)
		{
		case 0:	break;
		case '`':	ch = '~';	break;
		case '1':	ch = '!';	break;
		case '2':	ch = '@';	break;
		case '3':	ch = '#';	break;
		case '4':	ch = '$';	break;
		case '5':	ch = '%';	break;
		case '6':	ch = '^';	break;
		case '7':	ch = '&';	break;
		case '8':	ch = '*';	break;
		case '9':	ch = '(';	break;
		case '0':	ch = ')';	break;
		case '-':	ch = '_';	break;
		case '=':	ch = '+';	break;
		case '[':	ch = '{';	break;
		case ']':	ch = '}';	break;
		case '\\':	ch = '|';	break;
		case ';':	ch = ':';	break;
		case '\'':	ch = '"';	break;
		case ',':	ch = '<';	break;
		case '.':	ch = '>';	break;
		case '/':	ch = '?';	break;
		default:
			if('a' <= ch && ch <= 'z')
				ch -= 0x20;
			break;
		}
	}

	if(gKB_Callback && ch != 0)	gKB_Callback(ch);
}

/**
 * \fn void KB_UpdateLEDs()
 * \brief Updates the status of the keyboard LEDs
 */
void KB_UpdateLEDs()
{
	Uint8	leds;

	leds = (gbKB_CapsState ? 4 : 0);

	while( inb(0x64) & 2 );	// Wait for bit 2 to unset
	outb(0x60, 0xED);	// Send update command

	while( inb(0x64) & 2 );	// Wait for bit 2 to unset
	outb(0x60, leds);
}

/**
 * \fn int KB_IOCtl(tVFS_Node *Node, int Id, void *Data)
 * \brief Calls an IOCtl Command
 */
int KB_IOCtl(tVFS_Node *Node, int Id, void *Data)
{
	switch(Id)
	{
	case DRV_IOCTL_TYPE:	return DRV_TYPE_KEYBOARD;
	case DRV_IOCTL_IDENT:	memcpy(Data, "KB\0\0", 4);	return 1;
	case DRV_IOCTL_VERSION:	return 0x100;
	case DRV_IOCTL_LOOKUP:	return 0;

	// Sets the Keyboard Callback
	case KB_IOCTL_SETCALLBACK:
		// Sanity Check
		if((Uint)Data < KERNEL_BASE)	return 0;
		// Can only be set once
		if(gKB_Callback != NULL)	return 0;
		// Set Callback
		gKB_Callback = Data;
		return 1;

	default:
		return 0;
	}
}
