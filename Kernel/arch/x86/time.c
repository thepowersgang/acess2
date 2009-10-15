/*
 * Acess2 Kernel
 * Timekeeping
 * arch/x86/time.c
 */
#include <common.h>

// === MACROS ===
#define	NUM_TIMERS	8
#define	TIMER_QUANTUM	100
#define TIMER_RATE	13	// (Max: 15, Min: 2) - 15 = 1Hz, 13 = 4Hz, 10 = 1024Hz
#define TIMER_FREQ	(32768>>TIMER_RATE)	//Hz
#define MS_PER_TICK_WHOLE	(1000/(TIMER_FREQ))
#define MS_PER_TICK_FRACT	((Uint64)(1000*TIMER_FREQ-((Uint64)MS_PER_TICK_WHOLE)*0x80000000/TIMER_FREQ))

// === TYPEDEFS ===
typedef struct sTimer {
	 int	FiresAfter;
	void	(*Callback)(void*);
	void	*Argument;
} tTimer;

// === PROTOTYPES ===
void	Time_Interrupt();
void	Timer_CallTimers();

// === GLOBALS ===
Uint64	giTicks = 0;
Sint64	giTimestamp = 0;
Uint64	giPartMiliseconds = 0;
tTimer	gTimers[NUM_TIMERS];

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
	
	//Log("giTimestamp = %lli", giTimestamp);
	
	//Timer_CallTimers();

	// Make sure the RTC Fires again
	outb(0x70, 0x0C); // Select register C
	inb(0x71);	// Just throw away contents.
}

/**
 * \fn void Time_TimerThread()
 */
#if 0
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

/**
 * \fn Sint64 now()
 * \brief Return the current timestamp
 */
Sint64 now()
{
	return giTimestamp;
}

/**
 * \fn void Timer_CallTimers()
 */
void Timer_CallTimers()
{
	 int	i;
	void	(*callback)(void *);
	
	for(i = 0;
		i < NUM_TIMERS;
		i ++)
	{
		//Log("Timer %i", i);
		if(gTimers[i].Callback == NULL)	continue;
		Log("%i - %lli < %lli", i, giTimestamp, gTimers[i].FiresAfter);
		if(giTimestamp < gTimers[i].FiresAfter)	continue;
		callback = gTimers[i].Callback;
		gTimers[i].Callback = NULL;
		callback(gTimers[i].Argument);
	}
}

/**
 * \fn int Time_CreateTimer(int Delta, void *Callback, void *Argument)
 */
int Time_CreateTimer(int Delta, void *Callback, void *Argument)
{
	 int	ret;
	
	if(Callback == NULL)	return -1;
	
	for(ret = 0;
		ret < NUM_TIMERS;
		ret++)
	{
		if(gTimers[ret].Callback != NULL)	continue;
		gTimers[ret].Callback = Callback;
		gTimers[ret].FiresAfter = giTimestamp + Delta;
		gTimers[ret].Argument = Argument;
		Log("Callback = %p", Callback);
		Log("Timer %i fires at %lli", ret, gTimers[ret].FiresAfter);
		return ret;
	}
	return -1;
}

/**
 * \fn void Time_RemoveTimer(int ID)
 */
void Time_RemoveTimer(int ID)
{
	if(ID < 0 || ID >= NUM_TIMERS)	return;
	gTimers[ID].Callback = NULL;
}

/**
 * \fn void Time_Delay(int Delay)
 * \brief Delay for a small ammount of time
 */
void Time_Delay(int Delay)
{
	Sint64	dest = giTimestamp + Delay;
	while(dest < giTimestamp)	Threads_Yield();
}
