/**
 * Acess2
 * - By John Hodge (thePowersGang)
 *
 * arch/arm7/debug.c
 * - ARM7 Debug output
 * NOTE: Currently designed for the realview-pb-a8 emulated by Qemu
 */
#include <acess.h>

// === CONSTANTS ===
//#define UART0_BASE	0x10009000
#define UART0_BASE	0xF1000000	// Boot time mapped

// === PROTOTYPES ===
void	KernelPanic_SetMode(void);
void	KernelPanic_PutChar(char Ch);
void	StartupPrint(const char *str);

// === GLOBALS ===
 int	giDebug_SerialInitialised = 0;

// === CODE ===
void Debug_PutCharDebug(char ch)
{
	if(ch == '\n')
		Debug_PutCharDebug('\r');

	#if PLATFORM_is_tegra2
	// Tegra2
	while( !(*(volatile Uint32*)(UART0_BASE + 0x14) & (1 << 5)) )
		;
	#endif
	
//	*(volatile Uint32*)(SERIAL_BASE + SERIAL_REG_DATA) = ch;
	*(volatile Uint32*)(UART0_BASE) = ch;
}

void Debug_PutStringDebug(const char *str)
{
	for( ; *str; str++ )
		Debug_PutCharDebug( *str );
}

void KernelPanic_SetMode(void)
{
}

void KernelPanic_PutChar(char ch)
{
//	Debug_PutCharDebug(ch);
}

void StartupPrint(const char *str)
{
}

