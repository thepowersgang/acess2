/**
 * Acess2
 * - By John Hodge (thePowersGang)
 *
 * arch/arm7/debug.c
 * - ARM7 Debug output
 * NOTE: Currently designed for the realview-pb-a8 emulated by Qemu
 * - PL011
 */
#include <acess.h>
#include <drv_serial.h>
#include <debug_hooks.h>

// === CONSTANTS ===
//#define UART0_BASE	0x10009000
#define UART0_BASE	0xF1000000	// Boot time mapped

// === PROTOTYPES ===
void	Debug_int_SerialIRQHandler(int IRQ, void *unused);
void	KernelPanic_SetMode(void);
void	KernelPanic_PutChar(char Ch);
void	StartupPrint(const char *str);

// === GLOBALS ===
 int	giDebug_SerialInitialised = 0;

// === CODE ===
void Debug_int_SerialIRQHandler(int IRQ, void *unused)
{
	volatile Uint32 *regs = (void*)UART0_BASE;
	#if PLATFORM_is_realview_pb
	if( !(regs[15] & 0x10) ) {
	#else
	if( !(regs[5] & 1) ) {
	#endif
		// RX Int hadn't fired
		Debug("No IRQ %x %x", regs[15], regs[0]);
		return ;
	}
	char ch = regs[0];
	Serial_ByteReceived(gSerial_KernelDebugPort, ch);
}

void Debug_PutCharDebug(char ch)
{
	if(ch == '\n')
		Debug_PutCharDebug('\r');
	
	volatile Uint32 *regs = (void*)UART0_BASE;
	
	if( !giDebug_SerialInitialised ) {
		#if PLATFORM_is_tegra2
		// 16550 (i.e. PC) compatible
		regs[1] = 5;	// Enable RX interrupt
		#else
		regs[14] = 0x10;	// Enable RX interrupt
		regs[13] = (1<<1);	// Set RX trigger to 1 byte
		#endif
		giDebug_SerialInitialised = 1;
	}

	#if PLATFORM_is_tegra2
	// Tegra2
	while( !(regs[5] & (1 << 5)) )
		;
	#endif
	
	regs[0] = ch;
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

void Proc_PrintBacktrace(void)
{
	// TODO: Print backtrace
}

