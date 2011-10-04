/*
 * Acess2
 * - By thePowersGang (John Hodge)
 *
 * PL050 (or comaptible) Driver
 */
#include <acess.h>
#include "common.h"


#define PL050_TXBUSY	0x20

// === PROTOTYPES ===
void	PL050_Init(Uint32 KeyboardBase, Uint8 KeyboardIRQ, Uint32 MouseBase, Uint8 MouseIRQ);
void	PL050_KeyboardHandler(int IRQ);
void	PL050_MouseHandler(int IRQ);
void	PL050_EnableMouse(void);
static inline void	PL050_WriteMouseData(Uint8 data);
static inline void	PL050_WriteKeyboardData(Uint8 data);
static inline Uint8	PL050_ReadMouseData(void);
static inline Uint8	PL050_ReadKeyboardData(void);

// === GLOBALS ===
Uint32	*gpPL050_KeyboardBase;
Uint32	*gpPL050_MouseBase;

// === CODE ===
void PL050_Init(Uint32 KeyboardBase, Uint8 KeyboardIRQ, Uint32 MouseBase, Uint8 MouseIRQ)
{
	if( KeyboardBase ) {
		gpPL050_KeyboardBase = MM_MapHW(KeyboardBase, 0x1000);
		IRQ_AddHandler(KeyboardIRQ, PL050_KeyboardHandler);
	}
	if( MouseBase ) {
		gpPL050_MouseBase = MM_MapHW(MouseBase, 0x1000);
		IRQ_AddHandler(MouseIRQ, PL050_MouseHandler);
	}
}

void PL050_KeyboardHandler(int IRQ)
{
	Uint8	scancode;

	scancode = PL050_ReadKeyboardData(0x60);
	KB_HandleScancode( scancode );
}

void PL050_MouseHandler(int IRQ)
{
	PS2Mouse_HandleInterrupt( PL050_ReadMouseData(0x60) );
}

void PL050_SetLEDs(Uint8 leds)
{
	PL050_WriteKeyboardData(0xED);
	PL050_WriteKeyboardData(leds);
}

void PL050_EnableMouse(void)
{
	Uint8	status;
	Log_Log("8042", "Enabling Mouse...");
	
	
	//PL050_WriteMouseData(0xD4);
	//PL050_WriteMouseData(0xF6);	// Set Default Settings
	PL050_WriteMouseData(0xD4);
	PL050_WriteMouseData(0xF4);	// Enable Packets
}

static inline void PL050_WriteMouseData(Uint8 Data)
{
	 int	timeout = 10000;
	while( --timeout && *(Uint32*)(MOUSE_BASE+1) & PL050_TXBUSY );
	if(timeout)
		*(Uint32*)(MOUSE_BASE+2) = Data;
	else
		Log_Error("PL050", "Write to mouse timed out");
}

static inline Uint8 PL050_ReadMouseData(void)
{
	return *(Uint32*)(MOUSE_BASE+2);
}
static inline void PL050_WriteKeyboardData(Uint8 data)
{
	 int	timeout = 10000;
	while( --timeout && *(Uint32*)(KEYBOARD_BASE+1) & PL050_TXBUSY );
	if(timeout)
		*(Uint32*)(KEYBOARD_BASE+2) = Data;
	else
		Log_Error("PL050", "Write to keyboard timed out");
}
static inline Uint8 PL050_ReadKeyboardData(void)
{
	return *(Uint32*)(MOUSE_BASE+2);
}

