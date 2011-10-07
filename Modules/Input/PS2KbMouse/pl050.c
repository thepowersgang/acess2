/*
 * Acess2
 * - By thePowersGang (John Hodge)
 *
 * PL050 (or comaptible) Driver
 */
#include <acess.h>
#include "common.h"

// === CONSTANTS ===
#define PL050_TXBUSY	0x20

// === PROTOTYPES ===
void	PL050_Init(Uint32 KeyboardBase, Uint8 KeyboardIRQ, Uint32 MouseBase, Uint8 MouseIRQ);
void	PL050_KeyboardHandler(int IRQ, void *Ptr);
void	PL050_MouseHandler(int IRQ, void *Ptr);
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
		gpPL050_KeyboardBase = (void*)MM_MapHWPages(KeyboardBase, 1);
		IRQ_AddHandler(KeyboardIRQ, PL050_KeyboardHandler, NULL);
	}
	if( MouseBase ) {
		gpPL050_MouseBase = (void*)MM_MapHWPages(MouseBase, 1);
		IRQ_AddHandler(MouseIRQ, PL050_MouseHandler, NULL);
	}
}

void PL050_KeyboardHandler(int IRQ, void *Ptr)
{
	Uint8	scancode;

	scancode = PL050_ReadKeyboardData();
	KB_HandleScancode( scancode );
}

void PL050_MouseHandler(int IRQ, void *Ptr)
{
	PS2Mouse_HandleInterrupt( PL050_ReadMouseData() );
}

void PL050_SetLEDs(Uint8 leds)
{
	PL050_WriteKeyboardData(0xED);
	PL050_WriteKeyboardData(leds);
}

void PL050_EnableMouse(void)
{
	Log_Log("PL050", "Enabling Mouse...");
	
	//PL050_WriteMouseData(0xD4);
	//PL050_WriteMouseData(0xF6);	// Set Default Settings
	PL050_WriteMouseData(0xD4);
	PL050_WriteMouseData(0xF4);	// Enable Packets
}

static inline void PL050_WriteMouseData(Uint8 Data)
{
	 int	timeout = 10000;

	if( !gpPL050_MouseBase ) {
		Log_Error("PL050", "Mouse disabled (gpPL050_MouseBase = NULL)");
		return ;
	}

	while( --timeout && gpPL050_MouseBase[1] & PL050_TXBUSY );
	if(timeout)
		gpPL050_MouseBase[2] = Data;
	else
		Log_Error("PL050", "Write to mouse timed out");
}

static inline Uint8 PL050_ReadMouseData(void)
{
	if( !gpPL050_MouseBase ) {
		Log_Error("PL050", "Mouse disabled (gpPL050_MouseBase = NULL)");
		return 0;
	}
	return gpPL050_MouseBase[2];
}
static inline void PL050_WriteKeyboardData(Uint8 Data)
{
	 int	timeout = 10000;

	if( !gpPL050_KeyboardBase ) {
		Log_Error("PL050", "Keyboard disabled (gpPL050_KeyboardBase = NULL)");
		return ;
	}

	while( --timeout && gpPL050_KeyboardBase[1] & PL050_TXBUSY );
	if(timeout)
		gpPL050_KeyboardBase[2] = Data;
	else
		Log_Error("PL050", "Write to keyboard timed out");
}
static inline Uint8 PL050_ReadKeyboardData(void)
{
	if( !gpPL050_KeyboardBase ) {
		Log_Error("PL050", "Keyboard disabled (gpPL050_KeyboardBase = NULL)");
		return 0;
	}

	return gpPL050_KeyboardBase[2];
}

