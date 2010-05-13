/*
 * Acess2 Kernel
 * Timekeeping
 * arch/x86/time.c
 */
#include <acess.h>

// === MACROS ===
#define	TIMER_QUANTUM	100
// 2^(15-rate), 15: 1HZ, 5: 1024Hz, 2: 8192Hz
#define TIMER_RATE	12	// (Max: 15, Min: 2) - 15 = 1Hz, 13 = 4Hz, 12 = 8Hz, 11 = 16Hz 10 = 32Hz, 2
#define TIMER_FREQ	(0x8000>>TIMER_RATE)	//Hz
#define MS_PER_TICK_WHOLE	(1000/(TIMER_FREQ))
#define MS_PER_TICK_FRACT	((Uint64)(1000*TIMER_FREQ-((Uint64)MS_PER_TICK_WHOLE)*0x80000000/TIMER_FREQ))

// === IMPORTS ===
extern Sint64	giTimestamp;
extern Uint64	giTicks;
extern Uint64	giPartMiliseconds;
extern void	Timer_CallTimers(void);

// === PROTOTYPES ===
void	Time_Interrupt();

// === CODE ===
/**
 * \fn int Time_Setup()
 * \brief Sets the system time from the Realtime-Clock
 */
int Time_Setup()
{
	Uint8	val;
	
	outb(0x70, inb(0x70)&0x7F);	// Disable NMIs
	__asm__ __volatile__ ("cli");	// Disable normal interrupts
	
	// Set IRQ8 firing rate
	outb(0x70, 0x0A);	// Set the index to register A
	val = inb(0x71); // Get the current value of register A
	outb(0x70, 0x0A); // Reset index to A
	val &= 0xF0;
	val |= TIMER_RATE;
	outb(0x71, val);	// Update the timer rate
		
	// Enable IRQ8
	outb(0x70, 0x0B);	// Set the index to register B
	val = inb(0x71);	// Read the current value of register B
	outb(0x70, 0x0B);	// Set the index again (a read will reset the index to register D)
	outb(0x71, val | 0x40);	// Write the previous value or'd with 0x40. This turns on bit 6 of register D
	
	__asm__ __volatile__ ("sti");	// Disable normal interrupts
	outb(0x70, inb(0x70)|0x80);	// Disable NMIs
	
	// Install IRQ Handler
	IRQ_AddHandler(8, Time_Interrupt);
	return 0;
}

/**
 * \fn void Time_Interrupt()
 * \brief Called on the timekeeping IRQ
 */
void Time_Interrupt()
{
	giTicks ++;
	giTimestamp += MS_PER_TICK_WHOLE;
	giPartMiliseconds += MS_PER_TICK_FRACT;
	if(giPartMiliseconds > 0x80000000) {
		giTimestamp ++;
		giPartMiliseconds -= 0x80000000;
	}
	
	Timer_CallTimers();

	// Make sure the RTC Fires again
	outb(0x70, 0x0C); // Select register C
	inb(0x71);	// Just throw away contents.
}

#if 0
/**
 * \fn void Time_TimerThread()
 */
void Time_TimerThread()
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
