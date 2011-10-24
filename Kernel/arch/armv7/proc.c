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
extern void	Proc_int_DropToUser(Uint32 IP, Uint32 SP) NORETURN;
extern Uint32	Proc_CloneInt(Uint32 *SP, Uint32 *MemPtr);
extern tVAddr	MM_NewKStack(int bGlobal);	// TODO: Move out into a header
extern tVAddr	MM_NewUserStack(void);
extern char	kernel_table0[];

// === PROTOTYPES ===
void	Proc_IdleThread(void *unused);

// === GLOBALS ===
tThread	*gpCurrentThread = &gThreadZero;
tThread *gpIdleThread = NULL;

// === CODE ===
void ArchThreads_Init(void)
{
	gThreadZero.MemState.Base = (tPAddr)&kernel_table0 - KERNEL_BASE;
}

void Proc_IdleThread(void *unused)
{
	Threads_SetPriority(gpIdleThread, -1);
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
	gpIdleThread->ThreadName = (char*)"Idle Thread";
}

int GetCPUNum(void)
{
	return 0;
}

tThread *Proc_GetCurThread(void)
{
	return gpCurrentThread;
}

void Proc_StartUser(Uint Entrypoint, Uint Base, int ArgC, char **ArgV, int DataSize)
{
	Uint32	*usr_sp;
	 int	i;
	char	**envp;
	tVAddr	delta;

	Log_Debug("Proc", "Proc_StartUser: (Entrypoint=%p, Base=%p, ArgC=%i, ArgV=%p, DataSize=0x%x)",
		Entrypoint, Base, ArgC, ArgV, DataSize);

	// Write data to the user's stack
	usr_sp = (void*)MM_NewUserStack();
	usr_sp -= (DataSize+3)/4;
	memcpy(usr_sp, ArgV, DataSize);
	free(ArgV);

	// Adjust user's copy of the arguments
	delta = (tVAddr)usr_sp -  (tVAddr)ArgV;
	Log("delta = %x", delta);
	ArgV = (void*)usr_sp;
	for(i = 0; ArgV[i]; i ++)	ArgV[i] += delta;
	envp = &ArgV[i+1];
	for(i = 0; envp[i]; i ++)	envp[i] += delta;
	Log("envp = %p", envp);
	
	*--usr_sp = (Uint32)envp;
	*--usr_sp = (Uint32)ArgV;
	*--usr_sp = (Uint32)ArgC;
	*--usr_sp = Base;
	
	// Drop to user code
	Log_Debug("Proc", "Proc_int_DropToUser(%p, %p)", Entrypoint, usr_sp);
	Proc_int_DropToUser(Entrypoint, (Uint32)usr_sp);
}

tTID Proc_Clone(Uint Flags)
{
	tThread	*new;
	Uint32	pc, sp, mem;

	new = Threads_CloneTCB(Flags);
	if(!new)	return -1;

	// Actual clone magic
	pc = Proc_CloneInt(&sp, &mem);
	if(pc == 0) {
		Log("Proc_Clone: In child");
		return 0;
	}
	
	new->SavedState.IP = pc;
	new->SavedState.SP = sp;
	new->MemState.Base = mem;

	Threads_AddActive(new);

	return new->TID;
}

tTID Proc_SpawnWorker( void (*Fnc)(void*), void *Ptr )
{
	tThread	*new;
	Uint32	sp;

	new = Threads_CloneThreadZero();
	if(!new)	return -1;
	if(new->ThreadName)	free(new->ThreadName);
	new->ThreadName = NULL;

	new->KernelStack = MM_NewKStack(1);
	if(!new->KernelStack) {
		// TODO: Delete thread
		Log_Error("Proc", "Unable to allocate kernel stack");
		return -1;
	}	

	sp = new->KernelStack;
	
	*(Uint32*)(sp -= 4) = (Uint)Ptr;
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
	free(new->ThreadName);
	new->ThreadName = NULL;

	// TODO: Non-shared stack
	new->KernelStack = MM_NewKStack(1);
	if(!new->KernelStack) {
		// TODO: Delete thread
		Log_Error("Proc", "Unable to allocate kernel stack");
		return -1;
	}	

	sp = new->KernelStack;
	
	*(Uint32*)(sp -= 4) = (Uint)Ptr;
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

	Log("Switching to %p (%i %s) IP=%p SP=%p TTBR0=%p",
		next, next->TID, next->ThreadName,
		next->SavedState.IP, next->SavedState.SP, next->MemState.Base
		);
	Log("Requested by %p", __builtin_return_address(0));
	
	gpCurrentThread = next;

	SwitchTask(
		next->SavedState.SP, &cur->SavedState.SP,
		next->SavedState.IP, &cur->SavedState.IP,
		next->MemState.Base
		);
	
}

void Proc_DumpThreadCPUState(tThread *Thread)
{
	
}

