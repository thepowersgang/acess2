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

// === PROTOTYPES ===
void	Threads_int_Init(void)	__attribute__((constructor(101)));
tThread	*Threads_int_CreateTCB(tThread *Parent);

// === GLOBALS ===
tThread	*gThreads_List;
tThread __thread	*lpThreads_This;
 int	giThreads_NextTID = 1;
tShortSpinlock	glThreadListLock;

// === CODE ===
void Threads_int_Init(void)
{
	lpThreads_This = Threads_int_CreateTCB(NULL);
}

tThread *Proc_GetCurThread(void)
{
	return lpThreads_This;
}

void Threads_PostEvent(tThread *Thread, Uint32 Events)
{
	if( !Thread ) {
		// nope.avi
		return ;
	}
	SHORTLOCK( &Thread->IsLocked );
	Thread->PendingEvents |= Events;
	if( Thread->WaitingEvents & Events )
		Threads_int_SemSignal(Thread->WaitSemaphore);
	SHORTREL( &Thread->IsLocked );
}

Uint32 Threads_WaitEvents(Uint32 Events)
{
	if( !Threads_int_ThreadingEnabled() ) {
		Log_Notice("Threads", "_WaitEvents: Threading disabled");
		return 0;
	}
	lpThreads_This->WaitingEvents = Events;
	Threads_int_SemWaitAll(lpThreads_This->WaitSemaphore);
	lpThreads_This->WaitingEvents = 0;
	Uint32	rv = lpThreads_This->PendingEvents;
	return rv;
}

void Threads_ClearEvent(Uint32 Mask)
{
	if( !Threads_int_ThreadingEnabled() ) {
		Log_Notice("Threads", "_ClearEvent: Threading disabled");
		return ;
	}
	SHORTLOCK(&lpThreads_This->IsLocked);
	lpThreads_This->PendingEvents &= ~Mask;
	SHORTREL(&lpThreads_This->IsLocked);
}

tUID Threads_GetUID(void) { return 0; }
tGID Threads_GetGID(void) { return 0; }

tTID Threads_GetTID(void) { return lpThreads_This ? lpThreads_This->TID : 0; }

int *Threads_GetMaxFD(void)        { return &lpThreads_This->Process->MaxFDs;  }
char **Threads_GetCWD(void)        { return &lpThreads_This->Process->CWD;     }
char **Threads_GetChroot(void)     { return &lpThreads_This->Process->Chroot;  }
void **Threads_GetHandlesPtr(void) { return &lpThreads_This->Process->Handles; }

void Threads_Yield(void)
{
	Log_KernelPanic("Threads", "Threads_Yield DEFINITELY shouldn't be used (%p)",
		__builtin_return_address(0));
}

void Threads_Sleep(void)
{
	Log_Warning("Threads", "Threads_Sleep shouldn't be used");
}

void Threads_int_WaitForStatusEnd(enum eThreadStatus Status)
{
	while( lpThreads_This->Status != Status )
		Threads_int_SemWaitAll(lpThreads_This->WaitSemaphore);
}

int Threads_SetName(const char *Name)
{
	if( !lpThreads_This )
		return 0;

	if( lpThreads_This->ThreadName )
		free(lpThreads_This->ThreadName);
	lpThreads_This->ThreadName = strdup(Name);

	return 0;
}

int *Threads_GetErrno(void) __attribute__ ((weak));

int *Threads_GetErrno(void)
{
	static int a_errno;
	return &a_errno;
}

tThread *Threads_RemActive(void)
{
	return lpThreads_This;
}

void Threads_AddActive(tThread *Thread)
{
	Thread->Status = THREAD_STAT_ACTIVE;
	// Increment state-change semaphore
	Threads_int_SemSignal(Thread->WaitSemaphore);
}

struct sProcess *Threads_int_CreateProcess(void)
{
	struct sProcess *ret = calloc(sizeof(struct sProcess), 1);

	ret->MaxFDs = 32;

	return ret;
}

tThread *Threads_int_CreateTCB(tThread *Parent)
{
	tThread	*ret = calloc( sizeof(tThread), 1 );
	ret->TID = giThreads_NextTID ++;
	ret->WaitSemaphore = Threads_int_SemCreate();
	//ret->Protector = Threads_int_MutexCreate();

	if( !Parent )
	{
		ret->Process = Threads_int_CreateProcess();
	}
	else
		ret->Process = Parent->Process;

	ret->ProcNext = ret->Process->Threads;
	ret->Process->Threads = ret;

	ret->Next = gThreads_List;
	gThreads_List = ret;
	
	return ret;
}

struct sThread *Proc_SpawnWorker(void (*Fcn)(void*), void *Data)
{
	if( !Threads_int_ThreadingEnabled() )
	{
		Log_Error("Threads", "Multithreading is disabled in this build");
		return NULL;
	}
	else
	{
		tThread	*ret = Threads_int_CreateTCB(lpThreads_This);
		ret->SpawnFcn = Fcn;
		ret->SpawnData = Data;
		Threads_int_CreateThread(ret);
		return ret;
	}
}

