/*
 * Acess2 x86_64 port
 * - Kernel Panic output
 */
#include <acess.h>

// === PROTOTYPES ===
void	KernelPanic_SetMode(void);
void	KernelPanic_PutChar(char ch);

// === GLOBALS ===
Uint16	*gpKernelPanic_Buffer = (void*)( KERNEL_BASE|0xB8000 );
 int	giKernelPanic_CurPos = 0;

// === CODE ===
void KernelPanic_SetMode(void)
{
	giKernelPanic_CurPos = 0;
}

void KernelPanic_PutChar(char ch)
{
	switch(ch)
	{
	case '\n':
		giKernelPanic_CurPos += 80;
	case '\r':
		giKernelPanic_CurPos /= 80;
		giKernelPanic_CurPos *= 80;
		break;
	
	default:
		if(' ' <= ch && ch <= 0x7F)
			gpKernelPanic_Buffer[giKernelPanic_CurPos] = 0x4F00|ch;
		giKernelPanic_CurPos ++;
		break;
	}
}
