/*
 * Acess2 Kernel
 * Timekeeping
 * arch/x86_64/time.c
 */
#include <acess.h>
#include <arch_config.h>

// === MACROS ===
#define	TIMER_QUANTUM	100
#define TIMER_FREQ	PIT_TIMER_BASE_N/(PIT_TIMER_BASE_D*PIT_TIMER_DIVISOR)
#define MS_PER_TICK_WHOLE	(1000*(PIT_TIMER_BASE_D*PIT_TIMER_DIVISOR)/PIT_TIMER_BASE_N)
#define MS_PER_TICK_FRACT	((0x80000000ULL*1000ULL*PIT_TIMER_BASE_D*PIT_TIMER_DIVISOR/PIT_TIMER_BASE_N)&0x7FFFFFFF)

// === IMPORTS ===
extern volatile Sint64	giTimestamp;
extern volatile Uint64	giTicks;
extern volatile Uint64	giPartMiliseconds;
extern void	Timer_CallTimers(void);

// === GLOBALS ===
volatile Uint64	giTime_TSCAtLastTick = 0;
volatile Uint64	giTime_TSCPerTick = 0;

// === PROTOTYPES ===
//Sint64	now(void);
 int	Time_Setup(void);
void	Time_UpdateTimestamp(void);
Uint64	Time_ReadTSC(void);

// === CODE ===
/**
 * \fn Sint64 now()
 * \brief Return the current timestamp
 */
Sint64 now(void)
{
	Uint64	tsc = Time_ReadTSC();
	tsc -= giTime_TSCAtLastTick;
	tsc *= MS_PER_TICK_WHOLE;
	if( giTime_TSCPerTick ) {
		tsc /= giTime_TSCPerTick;
	}
	else
		tsc = 0;
	return giTimestamp + tsc;
}

/**
 * \fn int Time_Setup(void)
 * \brief Sets the system time from the Realtime-Clock
 */
int Time_Setup(void)
{
	Log_Log("Timer", "PIT Timer firing at %iHz, %i.0x%08x miliseconds per tick",
		TIMER_FREQ, MS_PER_TICK_WHOLE, MS_PER_TICK_FRACT);

	// TODO: Read time from RTC
	
	return 0;
}

/**
 * \brief Called on the timekeeping IRQ
 */
void Time_UpdateTimestamp(void)
{
	Uint64	curTSC = Time_ReadTSC();
	
	if( giTime_TSCAtLastTick )
	{
		giTime_TSCPerTick = curTSC - giTime_TSCAtLastTick;
	}
	giTime_TSCAtLastTick = curTSC;
	
	giTicks ++;
	giTimestamp += MS_PER_TICK_WHOLE;
	giPartMiliseconds += MS_PER_TICK_FRACT;
	if(giPartMiliseconds > 0x80000000) {
		giTimestamp ++;
		giPartMiliseconds -= 0x80000000;
	}
	
	Timer_CallTimers();
}

#if 0
/**
 * \fn void Time_TimerThread(void)
 */
void Time_TimerThread(void)
{
	Sint64	next;
	Threads_SetName("TIMER");
	
	next = giTimestamp + TIMER_QUANTUM;
	for(;;)
	{
		while(giTimestamp < next)	Threads_Yield();
		next = giTimestamp + TIMER_QUANTUM;	
		Timer_CallTimers();
	}
}
#endif

Uint64 Time_ReadTSC(void)
{
	Uint32	a, d;
	__asm__ __volatile__ ("rdtsc" : "=a" (a), "=d" (d));
	return ((Uint64)d << 32) | a;
}
