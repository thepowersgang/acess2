/*
 * AcessOS Microkernel Version
 * irq.c
 */
#include <acess.h>

// === CONSTANTS ===
#define	MAX_CALLBACKS_PER_IRQ	4
#define TRACE_IRQS	0

// === TYPES ===
typedef void (*tIRQ_Callback)(int, void *);

// === PROTOTYPES ===
void	IRQ_Handler(tRegs *Regs);

// === GLOBALS ===
tIRQ_Callback	gIRQ_Handlers[16][MAX_CALLBACKS_PER_IRQ];
void	*gaIRQ_DataPointers[16][MAX_CALLBACKS_PER_IRQ];

// === CODE ===
/**
 * \fn void IRQ_Handler(tRegs *Regs)
 * \brief Handle an IRQ
 */
void IRQ_Handler(tRegs *Regs)
{
	 int	i, irq = Regs->int_num - 0xF0;
	 int	bHandled = 0;

	//Log("IRQ_Handler: (Regs={int_num:%i})", Regs->int_num);

	for( i = 0; i < MAX_CALLBACKS_PER_IRQ; i++ )
	{
		if( gIRQ_Handlers[irq][i] ) {
			gIRQ_Handlers[irq][i](irq, gaIRQ_DataPointers[irq][i]);
			#if TRACE_IRQS
			if( irq != 8 )
				Log("IRQ %i: Call %p", Regs->int_num, gIRQ_Handlers[Regs->int_num][i]);
			#endif
			bHandled = 1;
		}
	}
	
	if( !bHandled ) {
		Log_Debug("IRQ", "Unhandled IRQ %i", irq);
	}

	//Log(" IRQ_Handler: Resetting");
	if(irq >= 8)
		outb(0xA0, 0x20);	// ACK IRQ (Secondary PIC)
	outb(0x20, 0x20);	// ACK IRQ
	//Log("IRQ_Handler: RETURN");
}

/**
 * \fn int IRQ_AddHandler( int Num, void (*Callback)(int) )
 */
int IRQ_AddHandler( int Num, void (*Callback)(int, void*), void *Ptr )
{
	 int	i;
	for( i = 0; i < MAX_CALLBACKS_PER_IRQ; i++ )
	{
		if( gIRQ_Handlers[Num][i] == NULL ) {
			Log_Log("IRQ", "Added IRQ%i Cb#%i %p", Num, i, Callback);
			gIRQ_Handlers[Num][i] = Callback;
			gaIRQ_DataPointers[Num][i] = Ptr;
			return 1;
		}
	}

	Log_Warning("IRQ", "No free callbacks on IRQ%i", Num);
	return 0;
}
