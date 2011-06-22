/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * threads.c
 * - Thread and process handling
 */
#define _SIGNAL_H_	// Stop the acess signal.h being used
#define _HEAP_H_	// Stop heap.h being imported (collides with stdlib heap)
#define _VFS_EXT_H	// Stop vfs_ext.h being imported (collides with fd_set)
#undef CLONE_VM	// Such a hack
#include <arch.h>
#undef NULL	// Remove acess definition
#include <acess.h>
#include <mutex.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include "/usr/include/signal.h"
#include <SDL/SDL.h>

// === IMPORTS ===
void	VFS_CloneHandleList(int PID);

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
	struct sThread	*GlobalNext;
	struct sThread	*Next;

	 int	KernelTID;

	tTID	TID, PID;
	tUID	UID, GID;

	struct sThread	*Parent;

	char	*ThreadName;

	 int	State;	// 0: Dead, 1: Active, 2: Paused, 3: Asleep
	 int	ExitStatus;
	#if 0
	tState	CurState;
	#endif

	// Threads waiting for this thread to exit.
	// Quit logic:
	// - Wait for `WaitingThreads` to be non-null (maybe?)
	// - Wake first in the queue, wait for it to be removed
	// - Repeat
	// - Free thread and quit kernel thread
	struct sThread	*WaitingThreads;
	struct sThread	*WaitingThreadsEnd;

	// Config?
	Uint	Config[NUM_CFG_ENTRIES];
}	tThread;

// === PROTOTYPES ===
 int	Threads_Wake(tThread *Thread);

// === GLOBALS ===
tThread	gThreadZero = {
	State: 1,
	ThreadName: "ThreadZero"
};
tThread	*gpThreads = &gThreadZero;
__thread tThread	*gpCurrentThread = &gThreadZero;
 int	giThreads_NextThreadID = 1;

// === CODE ===
void Threads_Dump(void)
{
	tThread	*thread;
	for( thread = gpThreads; thread; thread = thread->GlobalNext )
	{
		Log_Log("Threads", "TID %i (%s), PID %i",
			thread->TID, thread->ThreadName, thread->PID);
		Log_Log("Threads", "User: %i, Group: %i",
			thread->UID, thread->GID);
		Log_Log("Threads", "Kernel Thread ID: %i",
			thread->KernelTID);
	}
}

void Threads_SetThread(int TID)
{
	tThread	*thread;
	for( thread = gpThreads; thread; thread = thread->GlobalNext )
	{
		if( thread->TID == TID ) {
			gpCurrentThread = thread;
			return ;
		}
	}
	Log_Error("Threads", "_SetThread - Thread %i is not on global list", TID);
}

tThread	*Threads_GetThread(int TID)
{
	tThread	*thread;
	for( thread = gpThreads; thread; thread = thread->GlobalNext )
	{
		if( thread->TID == TID )
			return thread;
	}
	return NULL;
}

/**
 * \brief Clone a thread control block (with a different TID)
 */
tThread *Threads_CloneTCB(tThread *TemplateThread)
{
	tThread	*ret = malloc(sizeof(tThread));
	
	memcpy(ret, TemplateThread, sizeof(tThread));
	
	ret->TID = giThreads_NextThreadID ++;
	
	ret->ThreadName = strdup(TemplateThread->ThreadName);
	
	ret->WaitingThreads = NULL;
	ret->WaitingThreadsEnd = NULL;
	
	// Add to the end of the queue
	// TODO: Handle concurrency issues
	ret->GlobalNext = gpThreads;
	gpThreads = ret;
	
	return ret;
}

tUID Threads_GetUID() { return gpCurrentThread->UID; }
tGID Threads_GetGID() { return gpCurrentThread->GID; }
tTID Threads_GetTID() { return gpCurrentThread->TID; }
tPID Threads_GetPID() { return gpCurrentThread->PID; }

int Threads_SetUID(int *Errno, tUID NewUID)
{
	if(Threads_GetUID() != 0) {
		if(Errno)	*Errno = -EACCES;
		return -1;
	}
	
	gpCurrentThread->UID = NewUID;
	return 0;
}

int Threads_SetGID(int *Errno, tGID NewGID)
{
	if(Threads_GetUID() != 0) {
		if(Errno)	*Errno = -EACCES;
		return -1;
	}
	
	gpCurrentThread->GID = NewGID;
	return 0;
}

Uint *Threads_GetCfgPtr(int Index)
{
//	Log_Debug("Threads", "Index=%i, gpCurrentThread=%p",
//		Index, gpCurrentThread);
	if( Index < 0 || Index >= NUM_CFG_ENTRIES )
		return NULL;
	if( !gpCurrentThread )
		return NULL;
	return &gpCurrentThread->Config[Index];
}

int Threads_WaitTID(int TID, int *Status)
{
	// Any Child
	if(TID == -1) {
		Log_Error("Threads", "TODO: Threads_WaitTID(TID=-1) - Any Child");
		return -1;
	}
	
	// Any peer/child thread
	if(TID == 0) {
		Log_Error("Threads", "TODO: Threads_WaitTID(TID=0) - Any Child/Sibling");
		return -1;
	}
	
	// TGID = abs(TID)
	if(TID < -1) {
		Log_Error("Threads", "TODO: Threads_WaitTID(TID<0) - TGID");
		return -1;
	}
	
	// Specific Thread
	if(TID > 0) {
		
		tThread	*thread = Threads_GetThread(TID);
		tThread	*us = gpCurrentThread;
		if(!thread)	return -1;
		
		us->Next = NULL;
		us->State = 3;
		if(thread->WaitingThreadsEnd)
		{
			thread->WaitingThreadsEnd->Next = us;
			thread->WaitingThreadsEnd = us;
		}
		else
		{
			thread->WaitingThreads = us;
			thread->WaitingThreadsEnd = us;
		}
		
		while(thread->State != 0)
			pause();
		
		if(Status)	*Status = thread->ExitStatus;
		thread->WaitingThreads = thread->WaitingThreads->Next;
		us->Next = NULL;
		
		return TID;
	}
	
	return 0;
}

void Threads_Sleep(void)
{
	// TODO: Add to a sleeping queue
	pause();
}

void Threads_Yield(void)
{
//	yield();
}

void Threads_Exit(int TID, int Status)
{
	tThread	*toWake;
	
//	VFS_Handles_Cleanup();
	
	#if 1
	// Wait for the thread to be waited upon
	while( gpCurrentThread->WaitingThreads == NULL )
		SDL_Delay(10);
	#endif
	
	while( (toWake = gpCurrentThread->WaitingThreads) )
	{
		Threads_Wake(toWake);
		
		while(gpCurrentThread->WaitingThreads == toWake)
			SDL_Delay(10);
	}
}

int Threads_Wake(tThread *Thread)
{
	kill( Thread->KernelTID, SIGUSR1 );
	return 0;
}

int Threads_WakeTID(tTID TID)
{
	tThread	*thread;
	thread = Threads_GetThread(TID);
	if( !thread )	return -1;
	return Threads_Wake(thread);
}

int Threads_CreateRootProcess(void)
{
	tThread	*thread = Threads_CloneTCB(&gThreadZero);
	thread->PID = thread->TID;
	
	// Handle list is created on first open
	
	return thread->PID;
}

int Threads_Fork(void)
{
	tThread	*thread = Threads_CloneTCB(gpCurrentThread);
	thread->PID = thread->TID;
	// Duplicate the VFS handles (and nodes) from vfs_handle.c
	
	VFS_CloneHandleList(thread->PID);
	
	return thread->PID;
}

int Mutex_Acquire(tMutex *Mutex)
{
	if(!Mutex->Protector.IsValid) {
		pthread_mutex_init( &Mutex->Protector.Mutex, NULL );
		Mutex->Protector.IsValid = 1;
	}
	pthread_mutex_lock( &Mutex->Protector.Mutex );
	return 0;
}

void Mutex_Release(tMutex *Mutex)
{
	pthread_mutex_unlock( &Mutex->Protector.Mutex );
}

void Semaphore_Init(tSemaphore *Sem, int InitValue, int MaxValue, const char *Module, const char *Name)
{
	memset(Sem, 0, sizeof(tSemaphore));
	// HACK: Use `Sem->Protector` as space for the semaphore pointer
	*(void**)(&Sem->Protector) = SDL_CreateSemaphore(InitValue);
}

int Semaphore_Wait(tSemaphore *Sem, int MaxToTake)
{
	SDL_SemWait( *(void**)(&Sem->Protector) );
	return 1;
}

int Semaphore_Signal(tSemaphore *Sem, int AmmountToAdd)
{
	 int	i;
	for( i = 0; i < AmmountToAdd; i ++ )
		SDL_SemPost( *(void**)(&Sem->Protector) );
	return AmmountToAdd;
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

