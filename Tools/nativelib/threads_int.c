/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * threads_int.c
 * - Internal threading functions
 */
#include <stddef.h>
#include <stdint.h>
#include <acess_logging.h>
#include <threads_int.h>
#include <pthread_weak.h>

// === CODE ===
void Threads_int_LockMutex(void *Mutex)
{
	if( pthread_mutex_lock )
	{
	}
	else
	{
		
	}
}

