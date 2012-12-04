/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * threads.c
 * - Thread and process handling
 */

#include <arch.h>
#include <acess.h>
#include <mutex.h>
#include "../../KernelLand/Kernel/include/semaphore.h"
typedef signed long long int	time_t;
#include "../../Usermode/Libraries/ld-acess.so_src/include_exp/acess/syscall_types.h"
#include <rwlock.h>
#include <events.h>
#include <threads_int.h>
#include <limits.h>
#include "include/threads_glue.h"

#define THREAD_EVENT_WAKEUP	0x80000000

// === IMPORTS ===
extern void	VFS_CloneHandleList(int PID);
extern void	VFS_CloneHandlesFromList(int PID, int nFD, int FDs[]);

// === STRUCTURES ===
// === PROTOTYPES ===
 int	Threads_Wake(tThread *Thread);

// === GLOBALS ===
tProcess gProcessZero = {
	.NativePID = 0,
	.CWD = "/",
	.Chroot = "/",
	.MaxFD = 100
};
tThread	gThreadZero = {
	.Status=THREAD_STAT_ACTIVE,
	.ThreadName="ThreadZero",
	.Process = &gProcessZero
};
tThread	*gpThreads = &gThreadZero;
__thread tThread	*gpCurrentThread = &gThreadZero;
 int	giThreads_NextThreadID = 1;

// === CODE ===
tThread *Proc_GetCurThread(void)
{
	return gpCurrentThread;
}

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

tThread	*Threads_GetThread(Uint TID)
{
	tThread	*thread;
	for( thread = gpThreads; thread; thread = thread->GlobalNext )
	{
		if( thread->TID == TID ) {
			return thread;
		}
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
	Threads_Glue_SemInit( &ret->EventSem, 0 );
	
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

int Threads_SetUID(tUID NewUID)
{
	if(Threads_GetUID() != 0) {
		errno = EACCES;
		return -1;
	}
	
	gpCurrentThread->UID = NewUID;
	return 0;
}

int Threads_SetGID(tGID NewGID)
{
	if(Threads_GetUID() != 0) {
		errno = -EACCES;
		return -1;
	}
	
	gpCurrentThread->GID = NewGID;
	return 0;
}

int *Threads_GetErrno(void) { return &gpCurrentThread->_errno; }
char **Threads_GetCWD(void) { return &gpCurrentThread->Process->CWD; }
char **Threads_GetChroot(void) { return &gpCurrentThread->Process->Chroot; }
int *Threads_GetMaxFD(void) { return &gpCurrentThread->Process->MaxFD; };

tTID Threads_WaitTID(int TID, int *Status)
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
		us->Status = THREAD_STAT_WAITING;
		// TODO: Locking
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
		
		Threads_WaitEvents( THREAD_EVENT_WAKEUP );
		
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
	//pause();
}

void Threads_Yield(void)
{
//	yield();
}

void Threads_Exit(int TID, int Status)
{
	tThread	*toWake;
	
//	VFS_Handles_Cleanup();

	gpCurrentThread->ExitStatus = Status;
	
	#if 1
	if( gpCurrentThread->Parent )
	{
		// Wait for the thread to be waited upon
		while( gpCurrentThread->WaitingThreads == NULL )
			Threads_Glue_Yield();
	}
	#endif
	
	while( (toWake = gpCurrentThread->WaitingThreads) )
	{
		Log_Debug("Threads", "Threads_Exit - Waking %p %i '%s'", toWake, toWake->TID, toWake->ThreadName);

		Threads_Wake(toWake);
		
		while(gpCurrentThread->WaitingThreads == toWake)
			Threads_Glue_Yield();
	}
}

int Threads_Wake(tThread *Thread)
{
	Thread->Status = THREAD_STAT_ACTIVE;
	Threads_PostEvent(Thread, THREAD_EVENT_WAKEUP);
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

int Threads_Spawn(int nFD, int FDs[], struct s_sys_spawninfo *info)
{
	tThread	*thread = Threads_CloneTCB(gpCurrentThread);
	thread->PID = thread->TID;
	if( info )
	{
		// TODO: PGID?
		//if( info->flags & SPAWNFLAG_NEWPGID )
		//	thread->PGID = thread->PID;
		if( info->gid && thread->UID == 0 )
			thread->GID = info->gid;
		if( info->uid && thread->UID == 0 )	// last because ->UID is used above
			thread->UID = info->uid;
	}
	
	VFS_CloneHandlesFromList(thread->PID, nFD, FDs);

	Log_Debug("Threads", "_spawn: %i", thread->PID);
	return thread->PID;
}

// --------------------------------------------------------------------
// Mutexes 
// --------------------------------------------------------------------
int Mutex_Acquire(tMutex *Mutex)
{
	Threads_Glue_AcquireMutex(&Mutex->Protector.Mutex);
	return 0;
}

void Mutex_Release(tMutex *Mutex)
{
	Threads_Glue_ReleaseMutex(&Mutex->Protector.Mutex);
}

// --------------------------------------------------------------------
// Semaphores
// --------------------------------------------------------------------
void Semaphore_Init(tSemaphore *Sem, int InitValue, int MaxValue, const char *Module, const char *Name)
{
	memset(Sem, 0, sizeof(tSemaphore));
	// HACK: Use `Sem->Protector` as space for the semaphore pointer
	Threads_Glue_SemInit( &Sem->Protector.Mutex, InitValue );
}

int Semaphore_Wait(tSemaphore *Sem, int MaxToTake)
{
	return Threads_Glue_SemWait( Sem->Protector.Mutex, MaxToTake );
}

int Semaphore_Signal(tSemaphore *Sem, int AmmountToAdd)
{
	return Threads_Glue_SemSignal( Sem->Protector.Mutex, AmmountToAdd );
}

// --------------------------------------------------------------------
// Event handling
// --------------------------------------------------------------------
Uint32 Threads_WaitEvents(Uint32 Mask)
{
	Uint32	rv;

	//Log_Debug("Threads", "Mask = %x, ->Events = %x", Mask, gpCurrentThread->Events);	

	gpCurrentThread->WaitMask = Mask;
	if( !(gpCurrentThread->Events & Mask) )
	{
		if( Threads_Glue_SemWait(gpCurrentThread->EventSem, INT_MAX) == -1 ) {
			Log_Warning("Threads", "Wait on eventsem of %p, %p failed",
				gpCurrentThread, gpCurrentThread->EventSem);
		}
		//Log_Debug("Threads", "Woken from nap (%i here)", SDL_SemValue(gpCurrentThread->EventSem));
	}
	rv = gpCurrentThread->Events & Mask;
	gpCurrentThread->Events &= ~Mask;
	gpCurrentThread->WaitMask = -1;

	//Log_Debug("Threads", "- rv = %x", rv);

	return rv;
}

void Threads_PostEvent(tThread *Thread, Uint32 Events)
{
	Thread->Events |= Events;
//	Log_Debug("Threads", "Trigger event %x (->Events = %p) on %p", Events, Thread->Events, Thread);

	if( Events == 0 || Thread->WaitMask & Events ) {
		Threads_Glue_SemSignal( Thread->EventSem, 1 );
//		Log_Debug("Threads", "Waking %p(%i %s)", Thread, Thread->TID, Thread->ThreadName);
	}
}

void Threads_ClearEvent(Uint32 EventMask)
{
	gpCurrentThread->Events &= ~EventMask;
}

