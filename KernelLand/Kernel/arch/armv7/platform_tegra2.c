/*
 * Acess2 Kernel ARMv7 Port
 * - By John Hodge (thePowersGang)
 *
 * platform_tegra2.c
 * - Tegra2 Core code
 */
#include <acess.h>
#include <timers.h>	// MicroSleep
#include "platform_tegra2.h"
#include "include/options.h"

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
void	Timer_IRQHandler_Timer2(int IRQ, void *_unused);
void	Timer_IRQHandler_Timer3(int IRQ, void *_unused);
void	Timer_IRQHandler_Timer4(int IRQ, void *_unused);
void	Time_Setup(void);
tTime	Time_GetTickOffset(void);	// TODO: move to header

// === GLOBALS ===
// - Addresses for the GIC to use
tPAddr	gGIC_InterfaceAddr = GICI_PADDR;
tPAddr	gGIC_DistributorAddr = GICD_PADDR;
// - Map of timer registers
volatile struct sTimersMap *gpTimersMap;
volatile struct sClockResetMap *gpClockResetMap;
// - Interrupt controller code commented out, because the Tegra2 also has a GIC
#if 0
struct sIRQMap	gpIRQMap;
#endif

// === CODE ===

// -- Timers --
void Timer_IRQHandler_SysClock(int IRQ, void *_unused)
{
	giTimestamp += 100;
	gpTimersMap->TMR1.PCR_0 = (1<<30);
}

void Timer_IRQHandler_Timer2(int IRQ, void *_unused)
{
	Log_Debug("Tegra2Tme", "Timer 2");
}
void Timer_IRQHandler_Timer3(int IRQ, void *_unused)
{
	Log_Debug("Tegra2Tme", "Timer 3");
}
void Timer_IRQHandler_Timer4(int IRQ, void *_unused)
{
	Log_Debug("Tegra2Tme", "Timer 4");
}

void Time_MicroSleep(Uint16 Microsecs)
{
	Uint32	cur_ts = gpTimersMap->TIMERUS.CNTR_1US;
	Uint32	tgt_ts = cur_ts + Microsecs;
	if( tgt_ts < cur_ts )
		while( gpTimersMap->TIMERUS.CNTR_1US > cur_ts )
			;
	while( gpTimersMap->TIMERUS.CNTR_1US < tgt_ts )
		;
}

tTime Time_GetTickOffset(void)
{
	return (gpTimersMap->TIMERUS.CNTR_1US/1000) % 100;
}

void Time_Setup(void)
{
	gpTimersMap = (void*)MM_MapHWPages(0x60005000, 1);
	gpClockResetMap = (void*)MM_MapHWPages(0x60006000, 1);
	
	// Timer 1 (used for system timekeeping)
	IRQ_AddHandler(0*32+0, Timer_IRQHandler_SysClock, NULL);
	IRQ_AddHandler(0*32+1, Timer_IRQHandler_Timer2, NULL);
	IRQ_AddHandler(1*32+9, Timer_IRQHandler_Timer3, NULL);
	IRQ_AddHandler(1*32+10, Timer_IRQHandler_Timer4, NULL);
	gpTimersMap->TMR1.PTV_0 = (1<<31)|(1<<30)|(100*1000-1);	// enable, periodic, 100 ms
	gpTimersMap->TMR1.PCR_0 = (1<<30);
	Log_Debug("Tegra2Tme", "TMR0.PCR = 0x%x", gpTimersMap->TMR1.PCR_0);

	// Disabled until IRQs work
	//gpClockResetMap->RST_Source = (1 << 5)|(0<<4)|(7);	// Full reset on watchdog timeout

	Log_Debug("Tegra2Tme", "TIMERUS_USEC_CFG = 0x%x", gpTimersMap->TIMERUS.USEC_CFG);
	Log_Debug("Tegra2Tme", "TIMERUS_CNTR_1US = 0x%x", gpTimersMap->TIMERUS.CNTR_1US);
	Log_Debug("Tegra2Tme", "TMR0.PCR = 0x%x", gpTimersMap->TMR1.PCR_0);
	Log_Debug("Tegra2Tme", "TMR0.PTV = 0x%x", gpTimersMap->TMR1.PTV_0);
	for( int i = 0; i < 5; i ++ ) {
		for( int j = 0; j < 1000*1000; j ++ )
		{
			__asm__ __volatile__ ("mov r0, r0");
			__asm__ __volatile__ ("mov r0, r0");
			__asm__ __volatile__ ("mov r0, r0");
		}
		Log_Debug("Tegra2Tme", "TMR0.PCR = 0x%x", gpTimersMap->TMR1.PCR_0);
	}
	Log_Debug("Tegra2Tme", "TMR0.PCR = 0x%x", gpTimersMap->TMR1.PCR_0);
	Log_Debug("Tegra2Tme", "GICC_HPPIR = 0x%x", *(Uint32*)(0xF0000000 + 0x18));
	Log_Debug("Tegra2Tme", "GICC_IAR = 0x%x", *(Uint32*)(0xF0000000 + 0xC));
	Log_Debug("Tegra2Tme", "GICD_ISPENDR0 = 0x%x", *(Uint32*)(0xF0001000 + 0x200 + 0*4));
	Log_Debug("Tegra2Tme", "GICD_ISPENDR1 = 0x%x", *(Uint32*)(0xF0001000 + 0x200 + 1*4));
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
