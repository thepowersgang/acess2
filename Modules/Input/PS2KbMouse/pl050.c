/*
 * Acess2
 * - By thePowersGang (John Hodge)
 *
 * PL050 (or comaptible) Driver
 */
#include <acess.h>
#include "common.h"

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

#define PL050_TXBUSY	0x20

// === PROTOTYPES ===
void	PL050_Init(void);
void	PL050_KeyboardHandler(int IRQ);
void	PL050_MouseHandler(int IRQ);
void	PL050_EnableMouse(void);
static inline void	PL050_WriteMouseData(Uint8 data);
static inline void	PL050_WriteKeyboardData(Uint8 data);
static inline Uint8	PL050_ReadMouseData(void);
static inline Uint8	PL050_ReadKeyboardData(void);

// === CODE ===
void PL050_Init(void)
{
	IRQ_AddHandler(KEYBOARD_IRQ, PL050_KeyboardHandler);
	IRQ_AddHandler(MOUSE_IRQ, PL050_MouseHandler);	// Set IRQ
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

