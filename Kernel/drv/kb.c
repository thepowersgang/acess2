/*
 * Acess2
 * PS2 Keyboard Driver
 */
#include <common.h>
#include <modules.h>
#include <fs_devfs.h>
#include <tpl_drv_common.h>
#include <tpl_drv_keyboard.h>
#include "kb_kbdus.h"

// === CONSTANTS ===
#define	KB_BUFFER_SIZE	1024

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
MODULE_DEFINE(0, 0x0100, PS2Keybard, KB_Install, NULL, NULL);
tDevFS_Driver	gKB_DevInfo = {
	NULL, "PS2Keyboard",
	{
	.NumACLs = 0,
	.Size = -1,
	.Read = KB_Read,
	.IOCtl = KB_IOCtl
	}
};
tKeybardCallback	gKB_Callback = NULL;
Uint8	**gpKB_Map = gpKBDUS;
Uint8	gbaKB_States[256];
 int	gbKB_ShiftState = 0;
 int	gbKB_CapsState = 0;
 int	gbKB_KeyUp = 0;
 int	giKB_KeyLayer = 0;
//Uint64	giKB_ReadBase = 0;
Uint8	gaKB_Buffer[KB_BUFFER_SIZE];	//!< Keyboard Ring Buffer
volatile int	giKB_InsertPoint = 0;	//!< Writing location marker
volatile int	giKB_ReadPoint = 0;	//!< Reading location marker
volatile int	giKB_InUse = 0;		//!< Lock marker

// === CODE ===
/**
 * \fn int KB_Install(char **Arguments)
 */
int KB_Install(char **Arguments)
{
	IRQ_AddHandler(1, KB_IRQHandler);
	DevFS_AddDevice( &gKB_DevInfo );
	return 1;
}

/**
 * \fn void KB_IRQHandler()
 * \brief Called on a keyboard IRQ
 */
void KB_IRQHandler()
{
	Uint8	scancode;
	Uint32	ch;
	 int	keyNum;

	//if( inportb(0x64) & 0x20 )	return;
	
	scancode = inb(0x60); // Read from the keyboard's data buffer

	// Ignore ACKs
	if(scancode == 0xFA) {
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
	keyNum = giKB_KeyLayer * 256 + scancode;
	// Check for unknown key
	if(!ch && !gbKB_KeyUp)
		Warning("UNK %i %x", giKB_KeyLayer, scancode);
	
	// Reset Layer
	giKB_KeyLayer = 0;
	
	// Key Up?
	if (gbKB_KeyUp)
	{
		gbKB_KeyUp = 0;
		gbaKB_States[ keyNum ] = 0;	// Unset key state flag
		
		if( !gbaKB_States[KEY_LSHIFT] && !gbaKB_States[KEY_RSHIFT] )
			gbKB_ShiftState = 0;
		
		KB_AddBuffer(KEY_KEYUP);
		KB_AddBuffer(ch);
		
		return;
	}

	// Set the bit relating to the key
	gbaKB_States[keyNum] = 1;
	if(ch == KEY_LSHIFT || ch == KEY_RSHIFT)
		gbKB_ShiftState = 1;
	
	// Check for Caps Lock
	if(ch == KEY_CAPSLOCK) {
		gbKB_CapsState = !gbKB_CapsState;
		KB_UpdateLEDs();
	}

	// Ignore Non-Printable Characters
	if(ch == 0 || ch & 0x80)		return;
		
	// Is shift pressed
	if(gbKB_ShiftState ^ gbKB_CapsState)
	{
		switch(ch)
		{
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
	
	// --- Check for Kernel Magic Combos
	if(gbaKB_States[KEY_LSHIFT] && gbaKB_States[KEY_RSHIFT])
	{
		switch(ch)
		{
		case 'D':	__asm__ __volatile__ ("xchg %bx, %bx");	break;
		case 'P':	Threads_Dump();	break;
		}
	}
	
	if(gKB_Callback)	gKB_Callback(ch);
}

/**
 * \fn void KB_AddBuffer(char ch)
 * \brief Add to the keyboard ring buffer
 */
void KB_AddBuffer(char ch)
{
	// Add to buffer
	gaKB_Buffer[ giKB_InsertPoint++ ] = ch;
	// - Wrap
	if( giKB_InsertPoint == KB_BUFFER_SIZE )	giKB_InsertPoint = 0;
	// - Force increment read pointer
	if( giKB_InsertPoint == giKB_ReadPoint )	giKB_ReadPoint ++;
}

/**
 * \fn Uint64 KB_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Dest)
 * \brief Read from the ring buffer
 * \param Node	Unused
 * \param Offset	Unused (Character Device)
 * \param Length	Number of bytes to read
 * \param Dest	Destination
 */
Uint64 KB_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Dest)
{
	 int	pos = 0;
	char	*dstbuf = Dest;
	
	if(giKB_InUse)	return -1;
	giKB_InUse = 1;
	
	while(giKB_ReadPoint != giKB_InsertPoint && pos < Length)
	{
		dstbuf[pos++] = gaKB_Buffer[ giKB_ReadPoint++ ];
		if( giKB_ReadPoint == KB_BUFFER_SIZE )	giKB_InsertPoint = 0;
	}
	
	giKB_InUse = 0;
	
	return Length;
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
