/*
 * Acess 2 Kernel
 * By John Hodge (thePowersGang)
 * - x86 Kernel Panic Handler
 */

#include <acess.h>

#define	FB	((Uint16 *)(KERNEL_BASE|0xB8000))

 int	giKP_Pos = 0;

/**
 * \brief Sets the screen mode for a kernel panic
 */
void KernelPanic_SetMode()
{
	 int	i;
	// Restore VGA 0xB8000 text mode
	
	// Clear Screen
	for( i = 0; i < 80*25; i++ )
	{
		FB[i] = 0x4F00;
	}
}

void KernelPanic_PutChar(char Ch)
{
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
}
