/*
 * Acess2 Kernel ARMv7 Port
 * - By John Hodge (thePowersGang)
 *
 * platform_tegra2.c
 * - Tegra2 Core code
 */
#include <acess.h>
#include "platform_tegra2.h"

// === CONSTANTS ===
#define TIMER0_INT	(0*32+0)	// Pri #0
#define TIMER1_INT	(0*32+1)	// Pri #1
#define TIMER2_INT	(1*32+9)	// Sec #9
#define TIMER3_INT	(1*32+10)	// Sec #10

// === Imports ===
extern volatile Sint64	giTimestamp;
extern volatile Uint64	giTicks;
extern volatile Uint64	giPartMiliseconds;
extern void	Timer_CallTimers(void);

// === PROTORTYPES ===
void	Timer_IRQHandler_SysClock(int IRQ, void *_unused);
void	Time_Setup(void);

// === GLOBALS ===
// - Addresses for the GIC to use
tPAddr	gGIC_InterfaceAddr = 0x50040000;
tPAddr	gGIC_DistributorAddr = 0x50041000;
// - Map of timer registers
struct sTimersMap *gpTimersMap;
// - Interrupt controller code commented out, because the Tegra2 also has a GIC
#if 0
struct sIRQMap	gpIRQMap;
#endif

// === CODE ===

// -- Timers --
void Timer_IRQHandler_SysClock(int IRQ, void *_unused)
{
	giTimestamp += 100;
	gpTimersMap->TMR0.PCR_0 = (1<<31);
}

void Time_Setup(void)
{
	gpTimersMap = (void*)MM_MapHWPages(0x60005000, 1);
	// Timer 1 (used for system timekeeping)
	IRQ_AddHandler(0*32+0, Timer_IRQHandler_SysClock, NULL);
	gpTimersMap->TMR0.PTV_0 = (1<31)|(1<30)|(100*1000);	// enable, periodic, 100 ms
}

#if 0
// -- Interrupt Controller --
void IRQ_CtrlrHandler(struct sIRQRegs *Ctrlr, int Ofs)
{
	// Primary CPU only?
	// TODO: 
}

void IRQ_RootHandler(void)
{
	IRQ_CtrlrHandler(&gpIRQMap->Pri, 0*32);
	IRQ_CtrlrHandler(&gpIRQMap->Sec, 1*32);
	IRQ_CtrlrHandler(&gpIRQMap->Tri, 2*32);
	IRQ_CtrlrHandler(&gpIRQMap->Quad, 3*32);
}

void IRQ_Setup(void)
{
	gpIRQMap = (void*)MM_MapHWPages(0x60004000, 1);
	
	gpIRQHandler = IRQ_RootHandler;
}
#endif
