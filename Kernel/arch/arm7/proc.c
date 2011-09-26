/*
 * Acess2
 * - By John Hodge (thePowersGang)
 *
 * arch/arm7/proc.
 * - ARM7 Process Switching
 */
#include <acess.h>
#include <threads_int.h>
#include <hal_proc.h>

// === IMPORTS ===
extern tThread	gThreadZero;

// === PROTOTYPES ===

// === GLOBALS ===
tThread	*gpCurrentThread = &gThreadZero;

// === CODE ===
void ArchThreads_Init(void)
{
}

void Proc_Start(void)
{
}

int GetCPUNum(void)
{
	return 0;
}

tThread *Proc_GetCurThread(void)
{
	return gpCurrentThread;
}

tTID Proc_Clone(Uint Flags)
{
	return -1;
}

void Proc_StartUser(Uint Entrypoint, Uint *Bases, int ArgC, char **ArgV, char **EnvP, int DataSize)
{
}

tTID Proc_SpawnWorker( void (*Fnc)(void*), void *Ptr )
{
	return 0;
}

void Proc_CallFaultHandler(tThread *Thread)
{

}

void Proc_Reschedule(void)
{
	// TODO: Task switching!
}

void Proc_DumpThreadCPUState(tThread *Thread)
{
	
}

