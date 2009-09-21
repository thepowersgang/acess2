/*
 * AcessOS Microkernel Version
 * irq.c
 */
#include <common.h>

// === CONSTANTS ===
#define	MAX_CALLBACKS_PER_IRQ	4

// === TYPES ===
typedef void (*tIRQ_Callback)(void);

// === GLOBALS ===
tIRQ_Callback	gIRQ_Handlers[16][MAX_CALLBACKS_PER_IRQ];

// === CODE ===
/**
 * \fn void IRQ_Handler(tRegs *Regs)
 * \brief Handle an IRQ
 */
void IRQ_Handler(tRegs *Regs)
{
	 int	i;
	
	Regs->int_num -= 0xF0;	// Adjust
	
	//Log("IRQ_Handler: (Regs={int_num:%i})", Regs->int_num);
	
	for( i = 0; i < MAX_CALLBACKS_PER_IRQ; i++ )
	{
		//Log(" IRQ_Handler: Call %p", gIRQ_Handlers[Regs->int_num][i]);
		if( gIRQ_Handlers[Regs->int_num][i] )
			gIRQ_Handlers[Regs->int_num][i]();
	}
	
	outb(0x20, 0x20);	// ACK IRQ
	if(Regs->int_num >= 8)
		outb(0xA0, 0x20);	// ACK IRQ (Secondary PIC)
}

/**
 * \fn int IRQ_AddHandler( int Num, void (*Callback)(void) )
 */
int IRQ_AddHandler( int Num, void (*Callback)(void) )
{
	 int	i;
	for( i = 0; i < MAX_CALLBACKS_PER_IRQ; i++ )
	{
		if( gIRQ_Handlers[Num][i] == NULL ) {
			gIRQ_Handlers[Num][i] = Callback;
			return 1;
		}
	}
	
	return 0;
}
