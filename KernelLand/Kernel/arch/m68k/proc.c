/*
 * Acess2 M68K port
 * - By John Hodge (thePowersGang)
 *
 * arch/m68k/proc.c
 * - Multithreading
 */
#include <acess.h>
#include <threads_int.h>
#include <hal_proc.h>

// === IMPORTS ===
extern tThread	gThreadZero;

// === GLOBALS ===
tThread	*gpCurrentThread = &gThreadZero;

// === CODE ===
void ArchThreads_Init(void)
{
}

void Proc_Start(void)
{	
}

tThread *Proc_GetCurThread(void)
{
	return gpCurrentThread;
}

int GetCPUNum(void)
{
	return 0;
}

tTID Proc_Clone(Uint Flags)
{
	UNIMPLEMENTED();
	return -1;
}

tTID Proc_NewKThread(tThreadFunction Fcn, void *Arg)
{
	UNIMPLEMENTED();
	return -1;
}

tTID Proc_SpawnWorker(tThreadFunction Fcn, void *Arg)
{
	UNIMPLEMENTED();
	return -1;
}

void Proc_StartUser(Uint Entrypoint, Uint Base, int ArgC, char **ArgV, int DataSize)
{
	Log_KernelPanic("Proc", "TODO: Implement Proc_StartUser");
	for(;;);
}

void Proc_CallFaultHandler(tThread *Thread)
{
	
}

void Proc_DumpThreadCPUState(tThread *Thread)
{
	
}

void Proc_Reschedule(void)
{
	Log_Notice("Proc", "TODO: Implement Proc_Reschedule");
}

