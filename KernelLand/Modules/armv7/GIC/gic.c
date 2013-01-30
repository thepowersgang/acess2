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
#include <options.h>

#define N_IRQS	1024

// === IMPORTS ===
extern void	*gpIRQHandler;
extern tPAddr	gGIC_DistributorAddr;
extern tPAddr	gGIC_InterfaceAddr;

// === TYPES ===
typedef void (*tIRQ_Handler)(int, void*);

// === PROTOTYPES ===
 int	GIC_Install(char **Arguments);
void	GIC_IRQHandler(void);

// === GLOBALS ===
MODULE_DEFINE(0, 0x100, armv7_GIC, GIC_Install, NULL, NULL);
volatile Uint32	*gpGIC_DistributorBase;
volatile Uint32	*gpGIC_InterfaceBase;
tIRQ_Handler	gaIRQ_Handlers[N_IRQS];
void	*gaIRQ_HandlerData[N_IRQS];

// === CODE ===
int GIC_Install(char **Arguments)
{
	// Initialise
	Log_Debug("GIC", "Dist: %P, Interface: %P",
		gGIC_DistributorAddr, gGIC_InterfaceAddr);
	gpGIC_InterfaceBase = (void*)MM_MapHWPages(gGIC_InterfaceAddr, 1);
	LOG("gpGIC_InterfaceBase = %p", gpGIC_InterfaceBase);
	gpGIC_DistributorBase = (void*)MM_MapHWPages(gGIC_DistributorAddr, 1);
	LOG("gpGIC_DistributorBase = %p", gpGIC_DistributorBase);

	gpGIC_InterfaceBase[GICC_CTLR] = 0;	// Disable CPU interaface
	LOG("GICC_IAR = %x (CTLR=0)", gpGIC_InterfaceBase[GICC_IAR]);

	gpGIC_InterfaceBase[GICC_PMR] = 0xFF;	// Effectively disable prioritories
	gpGIC_InterfaceBase[GICC_CTLR] = 1;	// Enable CPU
	gpGIC_DistributorBase[GICD_CTLR] = 1;	// Enable Distributor

	gpIRQHandler = GIC_IRQHandler;

	__asm__ __volatile__ ("cpsie if");	// Enable IRQs and FIQs

#if 0
	for( int i = 0; i < N_IRQS/32; i ++ ) {
		Log_Debug("GIC", "GICD_ISENABLER%i %x = %08x",
			i, GICD_ISENABLER0 + i,
			gpGIC_DistributorBase[GICD_ISENABLER0+i]);
		gpGIC_DistributorBase[GICD_ISENABLER0+i] = 0;
	}
#endif

	#if 0
	// Testing - First 32 actual interrupts enabled
	gpGIC_DistributorBase[GICD_ISENABLER0+1] = 0xFFFFFFFF;
	for( int i = 0; i < 32/4; i ++ )
		gpGIC_DistributorBase[GICD_ITARGETSR0+8+i] = 0x01010101;
	#endif

	// Clear out pending IRQs.
	gpGIC_InterfaceBase[GICC_EOIR] = gpGIC_InterfaceBase[GICC_IAR];

	return MODULE_ERR_OK;
}

void GIC_IRQHandler(void)
{
	Uint32	num = gpGIC_InterfaceBase[GICC_IAR];
	Log_Debug("GIC", "IRQ 0x%x", num);
	gaIRQ_Handlers[num]( num, gaIRQ_HandlerData[num] );
	gpGIC_InterfaceBase[GICC_EOIR] = num;
}

int IRQ_AddHandler(int IRQ, tIRQ_Handler Handler, void *Ptr)
{
	if( IRQ < 0 || IRQ >= N_IRQS-32 ) {
		return 1;
	}
	
	IRQ += 32;	// 32 internal IRQs
	// - Enable IRQ, clear pending and send to CPU 1 only
	gpGIC_DistributorBase[GICD_ISENABLER0+IRQ/32] = 1 << (IRQ & (32-1));
	((Uint8*)&gpGIC_DistributorBase[GICD_ITARGETSR0])[IRQ] = 1;
	gpGIC_DistributorBase[GICD_ICPENDR0+IRQ/32] = 1 << (IRQ & (32-1));
	
	// TODO: Does the GIC need to handle IRQ sharing?
	if( gaIRQ_Handlers[IRQ] ) {
		Log_Warning("GIC", "IRQ %i already handled by %p, %p ignored",
			IRQ, gaIRQ_Handlers[IRQ], Handler);
		return 2;
	}
	
	gaIRQ_Handlers[IRQ] = Handler;
	gaIRQ_HandlerData[IRQ] = Ptr;
	
	Log_Debug("GIC", "IRQ %i handled by %p(%p)", IRQ, Handler, Ptr);

	// DEBUG! Trip the interrupt	
	gpGIC_DistributorBase[GICD_ISPENDR0+IRQ/32] = 1 << (IRQ & (32-1));
	return 0;
}

