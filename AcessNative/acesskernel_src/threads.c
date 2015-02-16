/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * threads.c
 * - Thread and process handling
 */
#define DEBUG	1
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
#include <stdbool.h>

#define THREAD_EVENT_WAKEUP	0x80000000

// === IMPORTS ===
extern void	VFS_CloneHandleList(int PID);
extern void	VFS_CloneHandlesFromList(int PID, int nFD, int FDs[]);
extern void	VFS_ClearHandles(int PID);

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
	for( tThread *thread = gpThreads; thread; thread = thread->GlobalNext )
	{
		Log_Log("Threads", "TID %i (%s), PID %i",
			thread->TID, thread->ThreadName, thread->Process->PID);
		Log_Log("Threads", "User: %i, Group: %i",
			thread->Process->UID, thread->Process->GID);
		Log_Log("Threads", "Kernel Thread ID: %i",
			thread->KernelTID);
	}
}

void Threads_SetThread(int TID, void *Client)
{
	for( tThread *thread = gpThreads; thread; thread = thread->GlobalNext )
	{
		if( thread->TID == TID ) {
			gpCurrentThread = thread;
			thread->ClientPtr = Client;
			return ;
		}
	}
	Log_Error("Threads", "_SetThread - Thread %i is not on global list", TID);
}

tThread	*Threads_GetThread(Uint TID)
{
	for( tThread *thread = gpThreads; thread; thread = thread->GlobalNext )
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
tThread *Threads_CloneTCB(tThread *TemplateThread, bool bNewProcess)
{
	tThread	*ret = malloc(sizeof(tThread));
	memcpy(ret, TemplateThread, sizeof(tThread));
	
	ret->TID = giThreads_NextThreadID ++;
	
	ret->ThreadName = strdup(TemplateThread->ThreadName);
	Threads_Glue_SemInit( &ret->EventSem, 0 );
	
	ret->WaitingThreads = NULL;
	ret->WaitingThreadsEnd = NULL;

	if( bNewProcess )
	{
		tProcess *proc = malloc( sizeof(tProcess) );
		memcpy(proc, ret->Process, sizeof(tProcess));
		proc->nThreads = 0;
		proc->CWD = strdup(proc->CWD);
		proc->Chroot = strdup(proc->Chroot);
		
		proc->PID = ret->TID;
		
		ret->Process = proc;
	}	

	ret->Process->nThreads ++;

	// Add to the end of the queue
	// TODO: Handle concurrency issues
	ret->GlobalNext = gpThreads;
	gpThreads = ret;
	
	return ret;
}

void Threads_int_Destroy(tThread *Thread)
{
	// Clear WaitingThreads
	
	Threads_Glue_SemDestroy(Thread->EventSem);
	free(Thread->ThreadName);
	Thread->Process->nThreads --;
}

void Threads_Terminate(void)
{
	tThread	*us = gpCurrentThread;
	tProcess *proc = us->Process;

	if( us->TID == proc->PID )
	{
		// If we're the process leader, then tear down the entire process
		VFS_ClearHandles(proc->PID);
		tThread	**next_ptr = &gpThreads;
		for( tThread *thread = gpThreads; thread; thread = *next_ptr )
		{
			if( thread->Process == proc ) {
				Threads_int_Destroy(thread);
			}
			else {
				next_ptr = &thread->Next;
			}
		}
	}
	else
	{
		// Just a lowly thread, remove from process
		Threads_int_Destroy(us);
	}
	
	if( proc->nThreads == 0 )
	{
		free(proc->Chroot);
		free(proc->CWD);
		free(proc);
	}
}

tUID Threads_GetUID() { return gpCurrentThread->Process->UID; }
tGID Threads_GetGID() { return gpCurrentThread->Process->GID; }
tTID Threads_GetTID() { return gpCurrentThread->TID; }
tPID Threads_GetPID() { return gpCurrentThread->Process->PID; }

int Threads_SetUID(tUID NewUID)
{
	if(Threads_GetUID() != 0) {
		errno = EACCES;
		return -1;
	}
	
	gpCurrentThread->Process->UID = NewUID;
	return 0;
}

int Threads_SetGID(tGID NewGID)
{
	if(Threads_GetUID() != 0) {
		errno = -EACCES;
		return -1;
	}
	
	gpCurrentThread->Process->GID = NewGID;
	return 0;
}

int *Threads_GetErrno(void) { return &gpCurrentThread->_errno; }
static tProcess *proc(tProcess *Proc) { return Proc ? Proc : gpCurrentThread->Process; }
char **Threads_GetCWD   (tProcess *Proc) { return &proc(Proc)->CWD; }
char **Threads_GetChroot(tProcess *Proc) { return &proc(Proc)->Chroot; }
int *Threads_GetMaxFD   (tProcess *Proc) { return &proc(Proc)->MaxFD; };

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
	if(TID > 0)
	{
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
		
		if(Status)	*Status = thread->RetStatus;
		thread->WaitingThreads = thread->WaitingThreads->Next;
		us->Next = NULL;
		
		return TID;
	}
	
	return 0;
}

void Threads_Exit(int TID, int Status)
{
	tThread	*toWake;
	
//	VFS_Handles_Cleanup();

	gpCurrentThread->RetStatus = Status;
	
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

void Threads_Sleep()
{
	gpCurrentThread->Status = THREAD_STAT_SLEEPING;
	Threads_int_WaitForStatusEnd(THREAD_STAT_SLEEPING);
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
	tThread	*thread = Threads_CloneTCB(&gThreadZero, true);
	
	// Handle list is created on first open
	
	return thread->Process->PID;
}

int Threads_Fork(void)
{
	tThread	*thread = Threads_CloneTCB(gpCurrentThread, true);

	// Duplicate the VFS handles (and nodes) from vfs_handle.c
	VFS_CloneHandleList(thread->Process->PID);
	
	return thread->Process->PID;
}

int Threads_Spawn(int nFD, int FDs[], struct s_sys_spawninfo *info)
{
	tThread	*thread = Threads_CloneTCB(gpCurrentThread, true);
	if( info )
	{
		// TODO: PGID?
		//if( info->flags & SPAWNFLAG_NEWPGID )
		//	thread->PGID = thread->PID;
		if( thread->Process->UID == 0 )
		{
			if( info->gid )
				thread->Process->GID = info->gid;
			if( info->uid )
				thread->Process->UID = info->uid;
		}
	}
	
	VFS_CloneHandlesFromList(thread->Process->PID, nFD, FDs);

	return thread->Process->PID;
}

// ----
// ----
void Threads_int_Terminate(tThread *Thread)
{
	Thread->RetStatus = -1;
	Threads_AddActive(Thread);
}

void Threads_int_WaitForStatusEnd(enum eThreadStatus Status)
{
	tThread	*us = Proc_GetCurThread();
	ASSERT(Status != THREAD_STAT_ACTIVE);
	ASSERT(Status != THREAD_STAT_DEAD);
	LOG("%i(%s) - %i", us->TID, us->ThreadName, Status);
	while( us->Status == Status )
	{
		if( Threads_Glue_SemWait(gpCurrentThread->EventSem, INT_MAX) == -1 ) {
			Log_Warning("Threads", "Wait on eventsem of %p, %p failed",
				gpCurrentThread, gpCurrentThread->EventSem);
			return ;
		}
		if( us->Status == Status )
			Log_Warning("Threads", "Thread %p(%i %s) rescheduled while in %s state",
				us, us->TID, us->ThreadName, casTHREAD_STAT[Status]);
	}
}

int Threads_int_Sleep(enum eThreadStatus Status, void *Ptr, int Num, tThread **ListHead, tThread **ListTail, tShortSpinlock *Lock)
{
	tThread	*us = Proc_GetCurThread();
	us->Next = NULL;
	// - Mark as sleeping
	us->Status = Status;
	us->WaitPointer = Ptr;
	us->RetStatus = Num;	// Use RetStatus as a temp variable
		
	// - Add to waiting
	if( ListTail ) {
		if(*ListTail) {
			(*ListTail)->Next = us;
		}
		else {
			*ListHead = us;
		}
		*ListTail = us;
	}
	else {
		*ListHead = us;
	}
	
	if( Lock ) {
		SHORTREL( Lock );
	}
	Threads_int_WaitForStatusEnd(Status);
	us->WaitPointer = NULL;
	return us->RetStatus;
}

void Threads_AddActive(tThread *Thread)
{
	LOG("%i(%s)", Thread->TID, Thread->ThreadName);
	Thread->Status = THREAD_STAT_ACTIVE;
	Threads_Glue_SemSignal(Thread->EventSem, 1);
}

// --------------------------------------------------------------------
// Signals
// --------------------------------------------------------------------
void Threads_PostSignal(int SigNum)
{
	Log_Error("Threads", "TODO: %s", __func__);
}
void Threads_SignalGroup(tPGID PGID, int SignalNum)
{
	Log_Error("Threads", "TODO: %s", __func__);
}

