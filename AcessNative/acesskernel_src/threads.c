/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * threads.c
 * - Thread and process handling
 */
#define _SIGNAL_H_
#undef CLONE_VM	// Such a hack
#include <acess.h>
#include <unistd.h>
#include <stdint.h>
#include "/usr/include/signal.h"

// === STRUCTURES ===
#if 0
typedef struct sState
{
	void	*CurState;
	Uint	SP, BP, IP;
}	tState;
#endif

typedef struct sThread
{
	struct sThread	*Next;

	 int	KernelTID;

	tTID	TID, PID;
	tUID	UID, GID;

	struct sThread	*Parent;

	char	*ThreadName;

	 int	State;	// 0: Dead, 1: Active, 2: Paused, 3: Asleep
	#if 0
	tState	CurState;
	#endif

	// Config?
	Uint	Config[NUM_CFG_ENTRIES];
}	tThread;

// === GLOBALS ===
tThread	*gpThreads;
__thread tThread	*gpCurrentThread;

// === CODE ===
tThread	*Threads_GetThread(int TID)
{
	tThread	*thread;
	for( thread = gpThreads; thread; thread = thread->Next )
	{
		if( thread->TID == TID )
			return thread;
	}
	return NULL;
}

tUID Threads_GetUID() { return gpCurrentThread->UID; }
tGID Threads_GetGID() { return gpCurrentThread->GID; }
tTID Threads_GetTID() { return gpCurrentThread->TID; }
tPID Threads_GetPID() { return gpCurrentThread->PID; }

Uint *Threads_GetCfgPtr(int Index)
{
	return &gpCurrentThread->Config[Index];
}

void Threads_Sleep(void)
{
	pause();
}

void Threads_Yield(void)
{
//	yield();
}

int Threads_WakeTID(tTID TID)
{
	tThread	*thread;
	thread = Threads_GetThread(TID);
	if( !thread )	return -1;
	kill( thread->KernelTID, SIGUSR1 );
	return 0;
}

void Mutex_Acquire(tMutex *Mutex)
{
	if(!Mutex->Protector.IsValid) {
		pthread_mutex_init( &Mutex->Protector.Mutex, NULL );
		Mutex->Protector.IsValid = 1;
	}
	pthread_mutex_lock( &Mutex->Protector.Mutex );
}

void Mutex_Release(tMutex *Mutex)
{
	pthread_mutex_unlock( &Mutex->Protector.Mutex );
}

#if 0
void Threads_Sleep()
{
	gpCurrentThread->State = 3;
	if( setjmp(&gpCurrentThread->CurState) == 0 ) {
		// Return to user wait
		// Hmm... maybe I should have a "kernel" thread for every "user" thread
	}
	else {
		// Just woken up, return
		return ;
	}
}

int SaveState(tState *To)
{
	Uint	ip;
	__asm__ __volatile__(
		"call 1f;\n\t"
		"1f:\n\t"
		"pop %%eax"
		: "=a" (ip)
		: );
	// If we just returned
	if(!ip)	return 1;

	To->IP = ip;
	__asm__ __volatile__ ("mov %%esp, %1" : "=r"(To->SP));
	__asm__ __volatile__ ("mov %%ebp, %1" : "=r"(To->BP));
}
#endif

