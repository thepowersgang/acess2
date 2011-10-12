/*
 * Acess2
 * - By John Hodge (thePowersGang)
 *
 * arch/arm7/proc.c
 * - ARM7 Process Switching
 */
#include <acess.h>
#include <threads_int.h>
#include <hal_proc.h>

// === IMPORTS ===
extern tThread	gThreadZero;
extern void	SwitchTask(Uint32 NewSP, Uint32 *OldSP, Uint32 NewIP, Uint32 *OldIP, Uint32 MemPtr);
extern void	KernelThreadHeader(void);	// Actually takes args on stack
extern void	Proc_CloneInt(Uint32 *SP, Uint32 *MemPtr);
extern tVAddr	MM_NewKStack(int bGlobal);	// TODO: Move out into a header

// === PROTOTYPES ===
void	Proc_IdleThread(void *unused);
tTID	Proc_NewKThread(void (*Fnc)(void*), void *Ptr);

// === GLOBALS ===
tThread	*gpCurrentThread = &gThreadZero;
tThread *gpIdleThread = NULL;

// === CODE ===
void ArchThreads_Init(void)
{
}

void Proc_IdleThread(void *unused)
{
	Threads_SetPriority(gpIdleThread, -1);
	Threads_SetName("Idle Thread");
	for(;;) {
		Proc_Reschedule();
		__asm__ __volatile__ ("wfi");
	}
}

void Proc_Start(void)
{
	tTID	tid;

	tid = Proc_NewKThread( Proc_IdleThread, NULL );
	gpIdleThread = Threads_GetThread(tid);
}

int GetCPUNum(void)
{
	return 0;
}

tThread *Proc_GetCurThread(void)
{
	return gpCurrentThread;
}

void Proc_StartUser(Uint Entrypoint, Uint *Bases, int ArgC, char **ArgV, char **EnvP, int DataSize)
{
	Log_Debug("Proc", "Proc_StartUser: (Entrypoint=%p, Bases=%p, ArgC=%i, ...)",
		Entrypoint, Bases, ArgC);
}

tTID Proc_Clone(Uint Flags)
{
	tThread	*new;

	new = Threads_CloneTCB(Flags);
	if(!new)	return -1;
	
	Log_Error("Proc", "TODO: Implement Proc_Clone");
	
	return -1;
}

tTID Proc_SpawnWorker( void (*Fnc)(void*), void *Ptr )
{
	tThread	*new;
	Uint32	sp;

	new = Threads_CloneThreadZero();
	if(!new)	return -1;

	new->KernelStack = MM_NewKStack(1);
	if(!new->KernelStack) {
		// TODO: Delete thread
		Log_Error("Proc", "Unable to allocate kernel stack");
		return -1;
	}	

	sp = new->KernelStack;
	
	*(Uint32*)(sp -= 4) = (Uint)Ptr;
	*(Uint32*)(sp -= 4) = 1;
	*(Uint32*)(sp -= 4) = (Uint)Fnc;
	*(Uint32*)(sp -= 4) = (Uint)new;

	new->SavedState.SP = sp;
	new->SavedState.IP = (Uint)KernelThreadHeader;

	Threads_AddActive(new);

	return new->TID;
}

tTID Proc_NewKThread( void (*Fnc)(void*), void *Ptr )
{
	tThread	*new;
	Uint32	sp;

	new = Threads_CloneTCB(0);
	if(!new)	return -1;

	// TODO: Non-shared stack
	new->KernelStack = MM_NewKStack(1);
	if(!new->KernelStack) {
		// TODO: Delete thread
		Log_Error("Proc", "Unable to allocate kernel stack");
		return -1;
	}	

	sp = new->KernelStack;
	
	*(Uint32*)(sp -= 4) = (Uint)Ptr;
	*(Uint32*)(sp -= 4) = 1;
	*(Uint32*)(sp -= 4) = (Uint)Fnc;
	*(Uint32*)(sp -= 4) = (Uint)new;

	new->SavedState.SP = sp;
	new->SavedState.IP = (Uint)KernelThreadHeader;

	Threads_AddActive(new);

	return new->TID;
}

void Proc_CallFaultHandler(tThread *Thread)
{

}

void Proc_Reschedule(void)
{
	tThread	*cur, *next;

	cur = gpCurrentThread;

	next = Threads_GetNextToRun(0, cur);
	if(!next)	next = gpIdleThread;
	if(!next || next == cur)	return;

	Log("Switching to %p (%i %s) IP=%p SP=%p", next, next->TID, next->ThreadName, next->SavedState.IP, next->SavedState.SP);
	Log("Requested by %p", __builtin_return_address(0));
	
	gpCurrentThread = next;
	// TODO: Change kernel stack?

	SwitchTask(
		next->SavedState.SP, &cur->SavedState.SP,
		next->SavedState.IP, &cur->SavedState.IP,
		next->MemState.Base
		);
	
}

void Proc_DumpThreadCPUState(tThread *Thread)
{
	
}

