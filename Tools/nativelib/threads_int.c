/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * threads_int.c
 * - Internal threading functions
 *
 * POSIX Mutex/Semaphore management
 * Wait state
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>	// printf debugging
#include <acess_logging.h>
#include <threads_int.h>
#include <pthread_weak.h>
#include <shortlock.h>

// === TYPES ===
struct sThreadIntMutex { int lock; };
struct sThreadIntSem { int val; };

// === PROTOTYPES ===

// === CODE ===
int Threads_int_ThreadingEnabled(void)
{
	return !!pthread_create;
}

tThreadIntMutex *Threads_int_MutexCreate(void)
{
	if( Threads_int_ThreadingEnabled() )
	{
		tThreadIntMutex	*ret = malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init( (void*)ret, NULL );
		return ret;
	}
	else
	{
		return calloc(sizeof(tThreadIntMutex), 1);
	}
}

void Threads_int_MutexLock(tThreadIntMutex *Mutex)
{
	if( !Mutex ) {
		return ;
	}
	if( Threads_int_ThreadingEnabled() )
	{
		pthread_mutex_lock( (void*)Mutex );
	}
	else
	{
		if( Mutex->lock )
			Log_KernelPanic("Threads", "Double mutex lock");
		Mutex->lock = 1;
	}
}

void Threads_int_MutexRelease(tThreadIntMutex *Mutex)
{
	if( !Mutex ) {
		return ;
	}

	if( Threads_int_ThreadingEnabled() )
	{
		pthread_mutex_unlock( (void*)Mutex );
	}
	else
	{
		if( !Mutex->lock )
			Log_Notice("Threads", "Release of non-locked mutex %p", Mutex);
		Mutex->lock = 0;
	}
}

tThreadIntSem *Threads_int_SemCreate(void)
{
	if( Threads_int_ThreadingEnabled() )
	{
		tThreadIntSem *ret = malloc(sizeof(sem_t));
		sem_init( (void*)ret, 0, 0 );
		return ret;
	}
	else
	{
		return calloc(sizeof(tThreadIntSem), 1);
	}
}

void Threads_int_SemSignal(tThreadIntSem *Sem)
{
	if( Threads_int_ThreadingEnabled() )
	{
		sem_post( (void*)Sem );
	}
	else
	{
		Sem->val ++;
	}
}

void Threads_int_SemWaitAll(tThreadIntSem *Sem)
{
	if( Threads_int_ThreadingEnabled() )
	{
		// TODO: Handle multiples
		sem_wait( (void*)Sem );
		while( sem_trywait((void*)Sem) )
			;
	}
	else
	{
		if( !Sem->val )
			Log_KernelPanic("Threads", "Waiting on empty semaphre %p", Sem);
		Sem->val = 0;
	}
}

void *Threads_int_ThreadRoot(void *ThreadPtr)
{
	tThread	*thread = ThreadPtr;
	lpThreads_This = thread;
	Log_Debug("Threads", "SpawnFcn: %p, SpawnData: %p", thread->SpawnFcn, thread->SpawnData);
	thread->SpawnFcn(thread->SpawnData);
	return NULL;
}

int Threads_int_CreateThread(tThread *Thread)
{
	if( Threads_int_ThreadingEnabled() )
	{
		pthread_t *pthread = malloc(sizeof(pthread_t));
		Thread->ThreadHandle = pthread;
		return pthread_create(pthread, NULL, &Threads_int_ThreadRoot, Thread);
	}
	else
	{
		Log_KernelPanic("Threads", "Link with pthreads to use threading");
		return -1;
	}
}

void SHORTLOCK(tShortSpinlock *Lock)
{
	if( Threads_int_ThreadingEnabled() )
	{
		if( !*Lock ) {
			*Lock = malloc(sizeof(pthread_mutex_t));
			pthread_mutex_init(*Lock, NULL);
		}
//		printf("%p: SHORTLOCK wait\n", lpThreads_This);
		pthread_mutex_lock(*Lock);
//		printf("%p: SHORTLOCK held %p\n", lpThreads_This, __builtin_return_address(0));
	}
	else
	{
		if(*Lock)	Log_KernelPanic("---", "Double short lock");
		*Lock = (void*)1;
	}
}

void SHORTREL(tShortSpinlock *Lock)
{
	if( Threads_int_ThreadingEnabled() )
	{
		pthread_mutex_unlock(*Lock);
//		printf("%p: SHORTLOCK rel\n", lpThreads_This);
	}
	else
	{
		if(!*Lock)	Log_Notice("---", "Short release when not held");
		*Lock = NULL;
	}
}

int CPU_HAS_LOCK(tShortSpinlock *Lock)
{
	if( Threads_int_ThreadingEnabled() )
	{
		Log_KernelPanic("---", "TODO: CPU_HAS_LOCK with threading enabled");
		return 0;
	}
	else
	{
		return 0;
	}
}
