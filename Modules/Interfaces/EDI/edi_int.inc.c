/*
 * AcessOS EDI Interface
 * - IRQ Class
 * 
 * By John Hodge (thePowersGang)
 * 
 * This file has been released into the public domain.
 * You are free to use it as you wish.
 */
#include "edi/edi.h"

// === TYPES ===
typedef struct {
	uint16_t	State;	// 0: Unallocated, 1: Allocated, 2: Initialised, (Bit 0x8000 set if in heap)
	uint16_t	Num;
	interrupt_handler_t	Handler;
} tEdiIRQ;

// === PROTOTYPES ===
void EDI_Int_IRQ_Handler(tRegs *Regs);

// === GLOBALS ===
tEdiIRQ	gEdi_IRQObjects[16];

// === FUNCTIONS ===
/**
 * \fn object_pointer Edi_Int_IRQ_Construct(void)
 * \brief Creates a new IRQ Object
 * \return	Pointer to object
 */
object_pointer Edi_Int_IRQ_Construct(void)
{
	 int	i;
	// Search for a free irq
	for( i = 0; i < 16; i ++ )
	{
		if(gEdi_IRQObjects[i].State)	continue;
		gEdi_IRQObjects[i].State = 1;
		gEdi_IRQObjects[i].Num = 0;
		gEdi_IRQObjects[i].Handler = NULL;
		return &gEdi_IRQObjects[i];
	}
	return NULL;
}

/**
 * \fn void Edi_Int_IRQ_Destruct(object_pointer Object)
 * \brief Destruct an IRQ Object
 * \param Object	Object to destroy
 */
void Edi_Int_IRQ_Destruct(object_pointer Object)
{
	tEdiIRQ	*obj;
	
	VALIDATE_PTR(Object,);
	obj = GET_DATA(Object);
	
	if( !obj->State )	return;
	
	if( obj->Handler )
		irq_uninstall_handler( obj->Num );
	
	if( obj->State & 0x8000 ) {	// If in heap, free
		free(Object);
	} else {	// Otherwise, mark as unallocated
		obj->State = 0;
	}
}

/**
 * \fn int32_t	Edi_Int_IRQ_InitInt(object_pointer Object, uint16_t Num, interrupt_handler_t Handler)
 * \brief Initialises an IRQ
 * \param Object	Object Pointer (this)
 * \param Num	IRQ Number to use
 * \param Handler	Callback for IRQ
 */
int32_t	Edi_Int_IRQ_InitInt(object_pointer Object, uint16_t Num, interrupt_handler_t Handler)
{
	tEdiIRQ	*obj;
	
	//LogF("Edi_Int_IRQ_InitInt: (Object=0x%x, Num=%i, Handler=0x%x)\n", Object, Num, Handler);
	
	VALIDATE_PTR(Object,0);
	obj = GET_DATA(Object);
	
	if( !obj->State )	return 0;
	
	if(Num > 15)	return 0;
	
	// Install the IRQ if a handler is passed
	if(Handler) {
		if( !irq_install_handler(Num, Edi_Int_IRQ_Handler) )
			return 0;
		obj->Handler = Handler;
	}
	
	obj->Num = Num;
	obj->State &= ~0x3FFF;
	obj->State |= 2;	// Set initialised flag
	return 1;
}

/**
 * \fn uint16_t Edi_Int_IRQ_GetInt(object_pointer Object)
 * \brief Returns the irq number associated with the object
 * \param Object	IRQ Object to get number from
 * \return IRQ Number
 */
uint16_t Edi_Int_IRQ_GetInt(object_pointer Object)
{
	tEdiIRQ	*obj;
	
	VALIDATE_PTR(Object,0);
	obj = GET_DATA(Object);
	
	if( !obj->State )	return 0;
	return obj->Num;
}

/**
 * \fn void EDI_Int_IRQ_SetHandler(object_pointer Object, interrupt_handler_t Handler)
 * \brief Set the IRQ handler for an IRQ object
 * \param Object	IRQ Object to alter
 * \param Handler	Function to use as handler
 */
void EDI_Int_IRQ_SetHandler(object_pointer Object, interrupt_handler_t Handler)
{
	tEdiIRQ	*obj;
	
	// Get Data Pointer
	VALIDATE_PTR(Object,);
	obj = GET_DATA(Object);
	
	// Sanity Check arguments
	if( !obj->State )	return ;
	
	// Only register the mediator if it is not already
	if( Handler && !obj->Handler )
		if( !irq_install_handler(obj->Num, Edi_Int_IRQ_Handler) )
			return ;
	obj->Handler = Handler;
}

/**
 * \fn void EDI_Int_IRQ_Return(object_pointer Object)
 * \brief Return from interrupt
 * \param Object	IRQ Object
 * \note Due to the structure of acess interrupts, this is a dummy
 */
void EDI_Int_IRQ_Return(object_pointer Object)
{
}

/**
 * \fn void Edi_Int_IRQ_Handler(struct regs *Regs)
 * \brief EDI IRQ Handler - Calls the handler 
 * \param Regs	Register state at IRQ call
 */
void Edi_Int_IRQ_Handler(struct regs *Regs)
{
	 int	i;
	for( i = 0; i < 16; i ++ )
	{
		if(!gEdi_IRQObjects[i].State)	continue;	// Unused, Skip
		if(gEdi_IRQObjects[i].Num != Regs->int_no)	continue;	// Another IRQ, Skip
		if(!gEdi_IRQObjects[i].Handler)	continue;	// No Handler, Skip
		gEdi_IRQObjects[i].Handler( Regs->int_no );	// Call Handler
		return;
	}
}


// === CLASS DECLARATION ===
static edi_function_declaration_t	scEdi_Int_Functions_IRQ[] = {
		{"int32_t", "init_interrupt", 1, 3, NULL, //scEdi_Int_Variables_IO[0],
			(function_pointer)Edi_Int_IRQ_InitInt
			},
		{"uint32_t", "interrupt_get_irq", 1, 1, NULL, //scEdi_Int_Variables_IO[1],
			(function_pointer)Edi_Int_IRQ_GetInt
			},
		{"void", "interrupt_set_handler", 1, 2, NULL, //scEdi_Int_Variables_IO[2],
			(function_pointer)Edi_Int_IRQ_GetInt
			},
		{"void", "interrupt_return", 1, 1, NULL, //scEdi_Int_Variables_IO[3],
			(function_pointer)Edi_Int_IRQ_GetInt
			}
	};
static edi_class_declaration_t	scEdi_Int_Class_IRQ = 
	{
		INTERRUPTS_CLASS, 1, 12,
		scEdi_Int_Functions_IRQ,
		Edi_Int_IRQ_Construct,
		Edi_Int_IRQ_Destruct,
		NULL
	};
