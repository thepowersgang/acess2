/*
 * Acess2 Kernel ARMv7 Port
 * - By John Hodge (thePowersGang)
 *
 * platform_realviewpb.c
 * - RealviewPB core code
 */
#include <acess.h>

// === IMPORTS ===
extern tTime	giTimestamp;

// === PROTOTYPES ===
void	Time_Setup(void);	// TODO: Should be in header
void	Time_int_RTCIRQHandler(int IRQ, void *unused);
void	Time_int_TimerIRQHandler(int IRQ, void *unused);
tTime	Time_GetTickOffset(void);	// TODO: Should be in header

// === GLOBALS ===
tTime	giTimeBase;
Uint32	giTime_PartMS;
tPAddr	gGIC_DistributorAddr = 0x1e001000;
tPAddr	gGIC_InterfaceAddr = 0x1e000000;

#define DIVISOR_ENC	1
#define DIVISOR_REAL	(1 << (4*DIVISOR_ENC))
#define TICKS_PER_SECOND	(1000000/DIVISOR_REAL)
#define TICKS_PER_IRQ	(TICKS_PER_SECOND/2)	// Tick twice a second
#define TICKS_PER_MS	(TICKS_PER_SECOND/1000)
#define MS_PER_IRQ	(TICKS_PER_IRQ*1000/TICKS_PER_SECOND)	// (TICKS_PER_IRQ/(TICKS_PER_SECOND/1000))
#define PART_PER_IRQ	(TICKS_PER_IRQ%(TICKS_PER_SECOND/1000))	// (TICKS_PER_IRQ%(TICKS_PER_SECOND/1000))

// RTC
#define PL031_BASE	0x10017000
#define PL031_IRQ	10
volatile struct {
	Uint32	DR;
	Uint32	MR;
	Uint32	LR;
	Uint32	CR;
	Uint32	IMSC;
	Uint32	IS;
	Uint32	MIS;
	Uint32	ICR;
} *gTime_PL031;
// Timer
#define SP804_BASE	0x10011000
#define SP805_IRQ	4
volatile struct {
	struct {
		Uint32	Load;
		Uint32	Value;
		Uint32	Control;
		Uint32	CIS;
		Uint32	RIS;
		Uint32	MIS;
		Uint32	BGLoad;
		Uint32	_unused2;
	} Timers[2];
} *gTime_SP804;

// === CODE ===
void Time_int_RTCIRQHandler(int IRQ, void *unused)
{
	if( !(gTime_PL031->IS & 1) ) {
		return ;
	}
	gTime_PL031->ICR = 1;	// Clear interrupt
//	giTimestamp = (Uint64)gTime_PL031->DR * 1000 - giTimeBase;
}

void Time_int_TimerIRQHandler(int IRQ, void *unused)
{
	if( !(gTime_SP804->Timers[0].RIS) ) {
		return ;
	}
	gTime_SP804->Timers[0].CIS = 1;
	giTime_PartMS += PART_PER_IRQ;
	giTimestamp += MS_PER_IRQ;
	while( giTime_PartMS > TICKS_PER_MS ) {
		giTime_PartMS -= TICKS_PER_MS;
		giTimestamp ++;
	}
}

void Time_Setup(void)
{
	// PL031 RTC
	gTime_PL031 = MM_MapHWPages(PL031_BASE, 1);
	//IRQ_AddHandler(PL031_IRQ, Time_int_RTCIRQHandler, NULL);
	gTime_PL031->IMSC = 1;
	gTime_PL031->ICR = 1;
//	giTimestamp = (Uint64)gTime_PL031->DR * 1000;
	
	// SP804 Timer (Contains two basic ARM timers, each at 1MHz base)
	gTime_SP804 = MM_MapHWPages(SP804_BASE, 1);
	// - Use the first to maintain the system clock
	gTime_SP804->Timers[0].Load = TICKS_PER_IRQ;
	//  > Enable,Periodic,IE,Div=1,32bit
	gTime_SP804->Timers[0].Control = (1<<7)|(1<<6)|(1<<5)|(DIVISOR_ENC<<2)|(1<<1);
	IRQ_AddHandler(SP805_IRQ, Time_int_TimerIRQHandler, NULL);
	gTime_SP804->Timers[0].CIS = 1;
	// - TODO: use the second for short events (and use RTC for long events)
}

tTime Time_GetTickOffset(void)
{
	if( !gTime_SP804 )
		return 0;
	// Want count of ms since last tick
	return (giTime_PartMS + gTime_SP804->Timers[0].Value) / TICKS_PER_MS;
}
