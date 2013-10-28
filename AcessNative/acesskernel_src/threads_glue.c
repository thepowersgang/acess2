/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * threads.c
 * - Thread and process handling
 */
#include "include/threads_glue.h"

#define _ACESS_H
typedef void	**tShortSpinlock;
#include <rwlock.h>

// - Native headers
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
//#include "/usr/include/signal.h"
#include <SDL/SDL.h>
#include <pthread.h>
#include <assert.h>

#define NORETURN	__attribute__((noreturn))
#include <logdebug.h>	// Kernel land, but uses standards

// === CODE ===
void Threads_Glue_Yield(void)
{
	SDL_Delay(10);
}

void Threads_Glue_AcquireMutex(void **Lock)
{
	if(!*Lock) {
		*Lock = malloc( sizeof(pthread_mutex_t) );
		pthread_mutex_init( *Lock, NULL );
	}
	pthread_mutex_lock( *Lock );
}

void Threads_Glue_ReleaseMutex(void **Lock)
{
	pthread_mutex_unlock( *Lock );
}

void Threads_Glue_SemInit(void **Ptr, int Val)
{
	*Ptr = SDL_CreateSemaphore(Val);
	if( !*Ptr ) {
		Log_Warning("Threads", "Semaphore creation failed - %s", SDL_GetError());
	}
}

int Threads_Glue_SemWait(void *Ptr, int Max)
{
	 int	have = 0;
	assert(Ptr);
	do {
		if( SDL_SemWait( Ptr ) == -1 ) {
			return -1;
		}
		have ++;
	} while( SDL_SemValue(Ptr) && have < Max );
	return have;
}

int Threads_Glue_SemSignal( void *Ptr, int AmmountToAdd )
{
	 int	i;
	for( i = 0; i < AmmountToAdd; i ++ )
		SDL_SemPost( Ptr );
	return AmmountToAdd;
}

// --------------------------------------------------------------------
// Event handling
// --------------------------------------------------------------------
int RWLock_AcquireRead(tRWLock *Lock)
{
	if( !Lock->ReaderWaiting ) {
		Lock->ReaderWaiting = malloc(sizeof(pthread_rwlock_t));
		pthread_rwlock_init( (void*)Lock->ReaderWaiting, 0 );
	}
	pthread_rwlock_rdlock( (void*)Lock->ReaderWaiting );
	return 0;
}
int RWLock_AcquireWrite(tRWLock *Lock)
{
	if( !Lock->ReaderWaiting ) {
		Lock->ReaderWaiting = malloc(sizeof(pthread_rwlock_t));
		pthread_rwlock_init( (void*)Lock->ReaderWaiting, 0 );
	}
	pthread_rwlock_wrlock( (void*)Lock->ReaderWaiting );
	return 0;
}
void RWLock_Release(tRWLock *Lock)
{
	pthread_rwlock_unlock( (void*)Lock->ReaderWaiting );
}

