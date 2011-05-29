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
extern void	Threads_ToggleTrace(int TID);
extern void	Threads_Dump(void);
extern void	Heap_Stats(void);

// === PROTOTYPES ===
 int	KB_Install(char **Arguments);
void	KB_IRQHandler(int IRQNum);
void	KB_AddBuffer(char ch);
Uint64	KB_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Dest);
void	KB_UpdateLEDs(void);
 int	KB_IOCtl(tVFS_Node *Node, int Id, void *Data);

// === GLOBALS ===
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
 int	giKB_MagicAddress = 0;
 int	giKB_MagicAddressPos = 0;
#endif

// === CODE ===
/**
 * \brief Install the keyboard driver
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
 * \brief Called on a keyboard IRQ
 * \param IRQNum	IRQ number (unused)
 */
void KB_IRQHandler(int IRQNum)
{
	Uint8	scancode;
	Uint32	ch;
	// int	keyNum;

	// Check port 0x64 to tell if this is from the aux port
	//if( inb(0x64) & 0x20 )	return;

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
		case '0':	case '1':	case '2':	case '3':
		case '4':	case '5':	case '6':	case '7':
		case '8':	case '9':	case 'a':	case 'b':
		case 'c':	case 'd':	case 'e':	case 'f':
			{
			char	str[4] = {'0', 'x', ch, 0};
			if(giKB_MagicAddressPos == BITS/4)	return;
			giKB_MagicAddress |= atoi(str) << giKB_MagicAddressPos;
			giKB_MagicAddressPos ++;
			}
			return;
		
		// Instruction Tracing
		case 't':
			Log("Toggle instruction tracing on %i\n", giKB_MagicAddress);
			Threads_ToggleTrace( giKB_MagicAddress );
			giKB_MagicAddress = 0;	giKB_MagicAddressPos = 0;
			return;
		
		// Thread List Dump
		case 'p':	Threads_Dump();	return;
		// Heap Statistics
		case 'h':	Heap_Stats();	return;
		// Dump Structure
		case 's':	return;
		}
	}
	#endif

	// Capitals required?
	if( (gbKB_ShiftState != 0) != (gbKB_CapsState != 0))
	{
		// TODO: Move this to the keyboard map header
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
 * \fn void KB_UpdateLEDs(void)
 * \brief Updates the status of the keyboard LEDs
 */
void KB_UpdateLEDs(void)
{
	Uint8	leds;

	leds = (gbKB_CapsState ? 4 : 0);

	while( inb(0x64) & 2 );	// Wait for bit 2 to unset
	outb(0x60, 0xED);	// Send update command

	while( inb(0x64) & 2 );	// Wait for bit 2 to unset
	outb(0x60, leds);
}

static const char	*csaIOCTL_NAMES[] = {DRV_IOCTLNAMES, DRV_KEYBAORD_IOCTLNAMES, NULL};

/**
 * \fn int KB_IOCtl(tVFS_Node *Node, int Id, void *Data)
 * \brief Calls an IOCtl Command
 */
int KB_IOCtl(tVFS_Node *Node, int Id, void *Data)
{
	switch(Id)
	{
	BASE_IOCTLS(DRV_TYPE_KEYBOARD, "KB", 0x100, csaIOCTL_NAMES);
	
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
