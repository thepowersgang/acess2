/*
 * Acess2 libnative (Kernel Simulation Library)
 * - By John Hodge (thePowersGang)
 *
 * semaphore.c
 * - Acess semaphores
 */
#include <acess.h>
#include <semaphore.h>

#if 0
TODO:: Rework kernel-land semaphore code to use events
- Allows it to be used here and be tested easier
#endif

// === CODE ===
void Semaphore_Init(tSemaphore *Sem, int InitValue, int MaxValue, const char *Module, const char *Name)
{

}

int Semaphore_Wait(tSemaphore *Sem, int MaxToTake)
{
	return 0;
}

int Semaphore_Signal(tSemaphore *Sem, int AmmountToAdd)
{
	return 0;
}

int Semaphore_GetValue(tSemaphore *Sem)
{
	return 0;
}
