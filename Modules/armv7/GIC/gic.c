/*
 * ARMv7 GIC Support
 * - By John Hodge (thePowersGang)
 * 
 * gic.c
 * - GIC Core
 */
#define DEBUG	1

#include <acess.h>
#include <modules.h>
#include "gic.h"

#define N_IRQS	1024

// === IMPORTS ===
extern void	*gpIRQHandler;

// === TYPES ===
typedef void (*tIRQ_Handler)(int, void*);

// === PROTOTYPES ===
 int	GIC_Install(char **Arguments);
void	GIC_IRQHandler(void);

// === GLOBALS ===
MODULE_DEFINE(0, 0x100, armv7_GIC, GIC_Install, NULL, NULL);
Uint32	*gpGIC_DistributorBase;
Uint32	*gpGIC_InterfaceBase;
tPAddr	gGIC_DistributorAddr;
tPAddr	gGIC_InterfaceAddr;
tIRQ_Handler	gaIRQ_Handlers[N_IRQS];
void	*gaIRQ_HandlerData[N_IRQS];

// === CODE ===
int GIC_Install(char **Arguments)
{
	// Realview PB
	gGIC_InterfaceAddr   = 0x1e000000;
	gGIC_DistributorAddr = 0x1e001000;

	// Initialise
	gpGIC_InterfaceBase = (void*)MM_MapHWPages(gGIC_InterfaceAddr, 1);
	LOG("gpGIC_InterfaceBase = %p", gpGIC_InterfaceBase);
	gpGIC_DistributorBase = (void*)MM_MapHWPages(gGIC_DistributorAddr, 1);
	LOG("gpGIC_DistributorBase = %p", gpGIC_DistributorBase);

	gpGIC_InterfaceBase[GICC_PMR] = 0xFF;	
	gpGIC_InterfaceBase[GICC_CTLR] = 1;	// Enable CPU
	gpGIC_DistributorBase[GICD_CTLR] = 1;	// Enable Distributor

	gpIRQHandler = GIC_IRQHandler;

	__asm__ __volatile__ ("cpsie if");	// Enable IRQs and FIQs

	return MODULE_ERR_OK;
}

void GIC_IRQHandler(void)
{
	Uint32	num = gpGIC_InterfaceBase[GICC_IAR];
//	Log_Debug("GIC", "IRQ 0x%x", num);
	gaIRQ_Handlers[num]( num, gaIRQ_HandlerData[num] );
	gpGIC_InterfaceBase[GICC_EOIR] = num;
}

int IRQ_AddHandler(int IRQ, tIRQ_Handler Handler, void *Ptr)
{
	if( IRQ < 0 || IRQ >= N_IRQS-32 ) {
		return 1;
	}
	
	LOG("IRQ = %i", IRQ);
	IRQ += 32;	// 32 internal IRQs
	LOG("IRQ = %i (after adjust)", IRQ);
	LOG("mask = 0x%x", 1 << (IRQ & (31-1)));
	gpGIC_DistributorBase[GICD_ISENABLER0+IRQ/32] = 1 << (IRQ & (32-1));
	((Uint8*)&gpGIC_DistributorBase[GICD_ITARGETSR0])[IRQ] = 1;
	
//	Log_Warning("GIC", "TODO: Implement IRQ_AddHandler");
	
	if( gaIRQ_Handlers[IRQ] )
		return 2;
	
	gaIRQ_Handlers[IRQ] = Handler;
	gaIRQ_HandlerData[IRQ] = Ptr;
	
	return 0;
}

