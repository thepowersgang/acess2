/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * threads_int.c
 * - Internal threading functions
 */
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <acess_logging.h>
#include <threads_int.h>
#include <pthread_weak.h>

// === TYPES ===
typedef struct sThread	tThread;
struct sThreadIntMutex { int lock; };
struct sThreadIntSem { int val; };

// === CODE ===
void Threads_int_MutexLock(tThreadIntMutex *Mutex)
{
	if( !Mutex ) {
		return ;
	}
	if( pthread_mutex_lock )
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

	if( pthread_mutex_unlock )
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

void Threads_int_SemSignal(tThreadIntSem *Sem)
{
	if( sem_wait )
	{
		sem_wait( (void*)Sem );
	}
	else
	{
		Sem->val ++;
	}
}

void Threads_int_SemWaitAll(tThreadIntSem *Sem)
{
	if( sem_post )
	{
		sem_post( (void*)Sem );
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
	thread->SpawnFcn(thread->SpawnData);
	return NULL;
}

int Threads_int_CreateThread(tThread *Thread)
{
	if( pthread_create )
	{
		pthread_t *pthread = malloc(sizeof(pthread_t));
		Thread->ThreadHandle = pthread;
		return pthread_create(pthread, NULL, Threads_int_ThreadRoot, Thread);
	}
	else
	{
		Log_KernelPanic("Threads", "Link with pthreads to use threading");
		return -1;
	}
}

