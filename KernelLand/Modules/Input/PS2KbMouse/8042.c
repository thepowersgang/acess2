/*
 * Acess2
 * - By thePowersGang (John Hodge)
 *
 * 8042 (or comaptible) Driver
 */
#include <acess.h>
#include "common.h"

// === PROTOTYPES ===
void	KBC8042_Init(void);
void	KBC8042_KeyboardHandler(int IRQ, void *Ptr);
void	KBC8042_MouseHandler(int IRQ, void *Ptr);
void	KBC8042_EnableMouse(void);
static inline void	KBC8042_SendDataAlt(Uint8 data);
static inline void	KBC8042_SendData(Uint8 data);
static inline Uint8	KBC8042_ReadData(void);
static void	KBC8042_SendMouseCommand(Uint8 cmd);

// === CODE ===
void KBC8042_Init(void)
{
	IRQ_AddHandler(1, KBC8042_KeyboardHandler, NULL);
	IRQ_AddHandler(12, KBC8042_MouseHandler, NULL);	// Set IRQ
	
	{
		// Attempt to get around a strange bug in Bochs/Qemu by toggling
		// the controller on and off
		Uint8 temp = inb(0x61);
		outb(0x61, temp | 0x80);
		outb(0x61, temp & 0x7F);
		inb(0x60);	// Clear keyboard buffer
	}
}

void KBC8042_KeyboardHandler(int IRQ, void *Ptr)
{
	Uint8	scancode = inb(0x60);
	KB_HandleScancode( scancode );
}

void KBC8042_MouseHandler(int IRQ, void *Ptr)
{
	PS2Mouse_HandleInterrupt( inb(0x60) );
}

void KBC8042_SetLEDs(Uint8 leds)
{
	while( inb(0x64) & 2 );	// Wait for bit 2 to unset
	outb(0x60, 0xED);	// Send update command

	while( inb(0x64) & 2 );	// Wait for bit 2 to unset
	outb(0x60, leds);
}

void KBC8042_EnableMouse(void)
{
	Uint8	status;
	Log_Log("8042", "Enabling Mouse...");
	
	// Enable AUX PS/2
	KBC8042_SendDataAlt(0xA8);
	
	// Enable AUX PS/2 (Compaq Status Byte)
	KBC8042_SendDataAlt(0x20);	// Send Command
	status = KBC8042_ReadData();	// Get Status
	status &= ~0x20;	// Clear "Disable Mouse Clock"
	status |= 0x02; 	// Set IRQ12 Enable
	KBC8042_SendDataAlt(0x60);	// Send Command
	KBC8042_SendData(status);	// Set Status
	
	//mouseSendCommand(0xF6);	// Set Default Settings
	KBC8042_SendMouseCommand(0xF4);	// Enable Packets
}

static inline void KBC8042_SendDataAlt(Uint8 data)
{
	int timeout=100000;
	while( timeout-- && inb(0x64) & 2 );	// Wait for Flag to clear
	outb(0x64, data);	// Send Command
}
static inline void KBC8042_SendData(Uint8 data)
{
	int timeout=100000;
	while( timeout-- && inb(0x64) & 2 );	// Wait for Flag to clear
	outb(0x60, data);	// Send Command
}
static inline Uint8 KBC8042_ReadData(void)
{
	int timeout=100000;
	while( timeout-- && (inb(0x64) & 1) == 0);	// Wait for Flag to set
	return inb(0x60);
}
static inline void KBC8042_SendMouseCommand(Uint8 cmd)
{
	KBC8042_SendDataAlt(0xD4);
	KBC8042_SendData(cmd);
}

