/**
 */
#include <acess.h>

// === CONSTANTS ===
#define SERIAL_BASE	0x16000000
#define SERIAL_REG_DATA	0x0
#define SERIAL_REG_FLAG	0x18
#define SERIAL_FLAG_FULL	0x20

// === PROTOTYPES ===
void	KernelPanic_SetMode(void);
void	KernelPanic_PutChar(char Ch);
void	StartupPrint(const char *str);

// === GLOBALS ===
 int	giDebug_SerialInitialised = 0;

// === CODE ===
void Debug_PutCharDebug(char ch)
{
	while( *(volatile Uint32*)(SERIAL_BASE + SERIAL_REG_FLAG) & SERIAL_FLAG_FULL )
		;
	
	*(volatile Uint32*)(SERIAL_BASE + SERIAL_REG_DATA) = ch;
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
	Debug_PutCharDebug(ch);
}

void StartupPrint(const char *str)
{
}

