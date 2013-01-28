/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * threads.c
 * - Threads handling
 */
#include <acess.h>
#include <threads.h>
#include <threads_int.h>

// === GLOBALS ===
 int	gbThreads_MultithreadingEnabled;
tThread __thread	*lpThreads_This;

// === CODE ===
tThread *Proc_GetCurThread(void)
{
	return lpThreads_This;
}

void Threads_PostEvent(tThread *Thread, Uint32 Events)
{
	Threads_int_LockMutex(Thread->Protector);
	Thread->PendingEvents |= Events;
	if( Thread->WaitingEvents & Events )
		Threads_int_SemaphoreSignal(Thread->WaitSemaphore);
	Threads_int_ReleaseMutex(Thread->Protector);
}

Uint32 Threads_WaitEvents(Uint32 Events)
{
	Thread->WaitingEvents = Events;
	Threads_int_SemaphoreWaitAll(Thread->WaitSemaphore);
	Thread->WaitingEvents = 0;
	Uint32	rv = Thread->PendingEvents;
	return rv;
}

void Threads_ClearEvent(Uint32 Mask)
{
	Threads_int_LockMutex(Thread->Protector);
	lpThreads_This->PendingEvents &= ~Mask;
	Threads_int_ReleaseMutex(Thread->Protector);
}

tUID Threads_GetUID(void) { return 0; }
tGID Threads_GetGID(void) { return 0; }

tTID Threads_GetTID(void) { return 0; }

int *Threads_GetMaxFD(void) { static int max_fd=32; return &max_fd; }
char **Threads_GetCWD(void) { static char *cwd; return &cwd; }
char **Threads_GetChroot(void) { static char *chroot; return &chroot; }

void Threads_Yield(void)
{
	Log_Warning("Threads", "Threads_Yield DEFINITELY shouldn't be used");
}

void Threads_Sleep(void)
{
	Log_Warning("Threads", "Threads_Sleep shouldn't be used");
}

int Threads_SetName(const char *Name)
{
	Log_Notice("Threads", "TODO: Threads_SetName('%s')", Name);
	return 0;
}

int *Threads_GetErrno(void) __attribute__ ((weak));

int *Threads_GetErrno(void)// __attribute__ ((weak))
{
	static int a_errno;
	return &a_errno;
}

struct sThread *Proc_SpawnWorker(void (*Fcn)(void*), void *Data)
{
	if( !gbThreads_MultithreadingEnabled )
	{
		Log_Error("Threads", "Multithreading is disabled in this build");
		return NULL;
	}
	else
	{
		tThread	*ret = malloc( sizeof(tThread) );
		ret->SpawnFcn = Fcn;
		ret->SpawnData = Data;
		Threads_int_CreateThread(ret);
		Log_Error("Threads", "TODO - Use pthreads to impliment Proc_SpawnWorker");
		return NULL;
	}
}

