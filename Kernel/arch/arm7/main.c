/*
 * Acess2
 *
 * ARM7 Entrypoint
 * arch/arm7/main.c
 */

// === IMPORTS ===
extern void	Interrupts_Setup(void);
extern void	MM_SetupPhys(void);

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
