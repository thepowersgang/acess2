/*
 * Acess 2 Kernel
 * - By John Hodge (thePowersGang)
 * 
 * kpanic.c
 * - x86 Kernel Panic Handler
 */

#include <acess.h>
#include <proc.h>

// === CONSTANTS ===
#define	FB	((Uint16 *)(KERNEL_BASE|0xB8000))
#define BGC	0x4F00	// White on Red
//#define BGC	0xC000	// Black on Bright Red
//#define BGC	0x1F00	// White on Blue (BSOD!)

// === IMPORTS ===
extern Uint32	GetEIP(void);
extern void	Error_Backtrace(Uint32 eip, Uint32 ebp);
#if USE_MP
extern void	MP_SendIPIVector(int CPU, Uint8 Vector);
extern int	giNumCPUs;
extern int	GetCPUNum(void);
#endif

// === PROTOTYPES ===
void	KernelPanic_SetMode(void);
void	KernelPanic_PutChar(char Ch);

// === CONSTANTS ===
const struct {
	Uint16	IdxPort;
	Uint16	DatPort;
	Uint8	Index;
	Uint8	Value;
}	caRegValues[] = {
	//{0x3C0, 0x3C0, 0x10, 0x0C},	// Mode Control (Blink Enabled)
	{0x3C0, 0x3C0, 0x10, 0x04},	// Mode Control (Blink Disabled)
	{0x3C0, 0x3C0, 0x11, 0x00},	// Overscan Register
	{0x3C0, 0x3C0, 0x12, 0x0F},	// Color Plane Enable
	{0x3C0, 0x3C0, 0x13, 0x08},	// Horizontal Panning
	{0x3C0, 0x3C0, 0x14, 0x00},	// Color Select
	{0    , 0x3C2, 0   , 0x67},	// Miscellaneous Output Register
	{0x3C4, 0x3C5, 0x01, 0x00},	// Clock Mode Register
	{0x3C4, 0x3C5, 0x03, 0x00},	// Character select
	{0x3C4, 0x3C5, 0x04, 0x07},	// Memory Mode Register
	{0x3CE, 0x3CF, 0x05, 0x10},	// Mode Register
	{0x3CE, 0x3CF, 0x06, 0x0E},	// Miscellaneous Register
	{0x3D4, 0x3D5, 0x00, 0x5F},	// Horizontal Total
	{0x3D4, 0x3D5, 0x01, 0x4F},	// Horizontal Display Enable End
	{0x3D4, 0x3D5, 0x02, 0x50},	// Horizontal Blank Start
	{0x3D4, 0x3D5, 0x03, 0x82},	// Horizontal Blank End
	{0x3D4, 0x3D5, 0x04, 0x55},	// Horizontal Retrace Start
	{0x3D4, 0x3D5, 0x05, 0x81},	// Horizontal Retrace End
	{0x3D4, 0x3D5, 0x06, 0xBF},	// Vertical Total
	{0x3D4, 0x3D5, 0x07, 0x1F},	// Overflow Register
	{0x3D4, 0x3D5, 0x08, 0x00},	// Preset row scan
	{0x3D4, 0x3D5, 0x09, 0x4F},	// Maximum Scan Line
	{0x3D4, 0x3D5, 0x10, 0x9C},	// Vertical Retrace Start
	{0x3D4, 0x3D5, 0x11, 0x8E},	// Vertical Retrace End
	{0x3D4, 0x3D5, 0x12, 0x8F},	// Vertical Display Enable End
	{0x3D4, 0x3D5, 0x13, 0x28},	// Logical Width
	{0x3D4, 0x3D5, 0x14, 0x1F},	// Underline Location
	{0x3D4, 0x3D5, 0x15, 0x96},	// Vertical Blank Start
	{0x3D4, 0x3D5, 0x16, 0xB9},	// Vertical Blank End
	{0x3D4, 0x3D5, 0x17, 0xA3}	// CRTC Mode Control
};
#define	NUM_REGVALUES	(sizeof(caRegValues)/sizeof(caRegValues[0]))

// === GLOBALS ===
 int	giKP_Pos = 0;

// === CODE ===
/**
 * \brief Sets the screen mode for a kernel panic
 */
void KernelPanic_SetMode(void)
{
	 int	i;

	__asm__ __volatile__ ("cli");	// Stop the processor!
	
	// This function is called by Panic(), but MM_PageFault and the
	// CPU exception handers also call it, so let's not clear the screen
	// twice
	if( giKP_Pos )	return ;
	
	// Restore VGA 0xB8000 text mode
	#if 0
	for( i = 0; i < NUM_REGVALUES; i++ )
	{
		// Reset Flip-Flop
		if( caRegValues[i].IdxPort == 0x3C0 )	inb(0x3DA);
		
		if( caRegValues[i].IdxPort )
			outb(caRegValues[i].IdxPort, caRegValues[i].Index);
		outb(caRegValues[i].DatPort, caRegValues[i].Value);
	}
	
	inb(0x3DA);
	outb(0x3C0, 0x20);
	#endif

	#if USE_MP
	// Send halt to all processors
	for( i = 0; i < giNumCPUs; i ++ )
	{
		if(i == GetCPUNum())	continue ;
		FB[i] = BGC|('A'+i);
		MP_SendIPIVector(i, 0xED);
	}
	#endif
	
	#if ENABLE_KPANIC_MODE
	// Clear Screen
	for( i = 0; i < 80*25; i++ )
	{
		FB[i] = BGC;
	}
	
	{
		Uint32	eip = GetEIP();
		Uint32	ebp;
		__asm__ __volatile__ ("mov %%ebp, %0" : "=r" (ebp));
		Error_Backtrace(eip, ebp);
	}
	#endif
}

void KernelPanic_PutChar(char Ch)
{
	#if ENABLE_KPANIC_MODE
	if( giKP_Pos > 80*25 )	return ;
	switch(Ch)
	{
	case '\t':
		do {
			FB[giKP_Pos] &= 0xFF00;
			FB[giKP_Pos++] |= ' ';
		} while(giKP_Pos & 7);
		break;
	
	case '\n':
		giKP_Pos += 80;
	case '\r':
		giKP_Pos -= giKP_Pos % 80;
		break;
	
	default:
		if(' ' <= Ch && Ch < 0x7F)
		{
			FB[giKP_Pos] &= 0xFF00;
			FB[giKP_Pos] |= Ch;
		}
		giKP_Pos ++;
		break;
	}
	#if 0
	{
		char	s[2] = {Ch,0};
		VT_int_PutString(gpVT_CurTerm, s);
	}
	#endif
	#endif // ENABLE_KPANIC_MODE
}
