/*
 * Acess2
 *
 * ARM7 Entrypoint
 * arch/arm7/main.c
 */
#include <acess.h>

// === IMPORTS ===
extern void	Interrupts_Setup(void);

// === PROTOTYPES ===
 int	kmain(void);

// === CODE ===
int kmain(void)
{
	Interrupts_Setup();
	
	MM_SetupPhys();
	
	//TODO: 
	for(;;);
}
