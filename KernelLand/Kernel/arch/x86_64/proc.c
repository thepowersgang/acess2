/*
 * Acess2 x86_64 port
 * proc.c
 */
#include <acess.h>
#include <proc.h>
#include <threads.h>
#include <threads_int.h>
#include <desctab.h>
#include <mm_virt.h>
#include <errno.h>
#if USE_MP
# include <mp.h>
#endif
#include <arch_config.h>
#include <hal_proc.h>

// === FLAGS ===
#define DEBUG_TRACE_SWITCH	0
#define BREAK_ON_SWITCH 	0	// Break into bochs debugger on a task switch

// === CONSTANTS ===

// === TYPES ===
typedef struct sCPU
{
	Uint8	APICID;
	Uint8	State;	// 0: Unavaliable, 1: Idle, 2: Active
	Uint16	Resvd;
	tThread	*Current;
	tThread	*IdleThread;
}	tCPU;

// === IMPORTS ===
extern tGDT	gGDT[];
extern void	APStartup(void);	// 16-bit AP startup code

extern Uint	GetRIP(void);	// start.asm
extern Uint	SaveState(Uint *RSP, Uint *Regs);
extern Uint	Proc_CloneInt(Uint *RSP, Uint *CR3);
extern void	NewTaskHeader(void);	// Actually takes cdecl args
extern void	Proc_InitialiseSSE(void);
extern void	Proc_SaveSSE(Uint DestPtr);
extern void	Proc_DisableSSE(void);

extern Uint64	gInitialPML4[512];	// start.asm
extern int	giNumCPUs;
extern int	giNextTID;
extern int	giTotalTickets;
extern int	giNumActiveThreads;
extern tThread	gThreadZero;
extern tProcess	gProcessZero;
extern void	Threads_Dump(void);
extern void	Proc_ReturnToUser(tVAddr Handler, tVAddr KStackTop, int Argument);
extern void	Time_UpdateTimestamp(void);
extern void	SwitchTasks(Uint NewSP, Uint *OldSP, Uint NewIP, Uint *OldIO, Uint CR3);

// === PROTOTYPES ===
//void	ArchThreads_Init(void);
#if USE_MP
void	MP_StartAP(int CPU);
void	MP_SendIPI(Uint8 APICID, int Vector, int DeliveryMode);
#endif
void	Proc_IdleTask(void *unused);
//void	Proc_Start(void);
//tThread	*Proc_GetCurThread(void);
// int	Proc_NewKThread(void (*Fcn)(void*), void *Data);
// int	Proc_Clone(Uint *Err, Uint Flags);
// int	Proc_SpawnWorker(void);
Uint	Proc_MakeUserStack(void);
//void	Proc_StartUser(Uint Entrypoint, Uint *Bases, int ArgC, char **ArgV, char **EnvP, int DataSize);
void	Proc_StartProcess(Uint16 SS, Uint Stack, Uint Flags, Uint16 CS, Uint IP) NORETURN;
 int	Proc_Demote(Uint *Err, int Dest, tRegs *Regs);
//void	Proc_CallFaultHandler(tThread *Thread);
//void	Proc_DumpThreadCPUState(tThread *Thread);
//void	Proc_Reschedule(void);
void	Proc_Scheduler(int CPU, Uint RSP, Uint RIP);

// === GLOBALS ===
//!\brief Used by desctab.asm in SyscallStub
const int ci_offsetof_tThread_KernelStack = offsetof(tThread, KernelStack);
// --- Multiprocessing ---
#if USE_MP
volatile int	giNumInitingCPUs = 0;
tMPInfo	*gMPFloatPtr = NULL;
tAPIC	*gpMP_LocalAPIC = NULL;
Uint8	gaAPIC_to_CPU[256] = {0};
#endif
tCPU	gaCPUs[MAX_CPUS] = {
	{.Current = &gThreadZero}
	};
tTSS	*gTSSs = NULL;
tTSS	gTSS0 = {0};
// --- Error Recovery ---
Uint32	gaDoubleFaultStack[1024];

// === CODE ===
/**
 * \fn void ArchThreads_Init(void)
 * \brief Starts the process scheduler
 */
void ArchThreads_Init(void)
{
	Uint	pos = 0;
	
	#if USE_MP
	tMPTable	*mptable;
	
	// Mark BSP as active
	gaCPUs[0].State = 2;
	
	// -- Initialise Multiprocessing
	// Find MP Floating Table
	// - EBDA/Last 1Kib (640KiB)
	for(pos = KERNEL_BASE|0x9F000; pos < (KERNEL_BASE|0xA0000); pos += 16) {
		if( *(Uint*)(pos) == MPPTR_IDENT ) {
			Log("Possible %p", pos);
			if( ByteSum((void*)pos, sizeof(tMPInfo)) != 0 )	continue;
			gMPFloatPtr = (void*)pos;
			break;
		}
	}
	// - Last KiB (512KiB base mem)
	if(!gMPFloatPtr) {
		for(pos = KERNEL_BASE|0x7F000; pos < (KERNEL_BASE|0x80000); pos += 16) {
			if( *(Uint*)(pos) == MPPTR_IDENT ) {
				Log("Possible %p", pos);
				if( ByteSum((void*)pos, sizeof(tMPInfo)) != 0 )	continue;
				gMPFloatPtr = (void*)pos;
				break;
			}
		}
	}
	// - BIOS ROM
	if(!gMPFloatPtr) {
		for(pos = KERNEL_BASE|0xE0000; pos < (KERNEL_BASE|0x100000); pos += 16) {
			if( *(Uint*)(pos) == MPPTR_IDENT ) {
				Log("Possible %p", pos);
				if( ByteSum((void*)pos, sizeof(tMPInfo)) != 0 )	continue;
				gMPFloatPtr = (void*)pos;
				break;
			}
		}
	}
	
	// If the MP Table Exists, parse it
	if(gMPFloatPtr)
	{
		 int	i;
	 	tMPTable_Ent	*ents;
		Log("gMPFloatPtr = %p", gMPFloatPtr);
		Log("*gMPFloatPtr = {");
		Log("\t.Sig = 0x%08x", gMPFloatPtr->Sig);
		Log("\t.MPConfig = 0x%08x", gMPFloatPtr->MPConfig);
		Log("\t.Length = 0x%02x", gMPFloatPtr->Length);
		Log("\t.Version = 0x%02x", gMPFloatPtr->Version);
		Log("\t.Checksum = 0x%02x", gMPFloatPtr->Checksum);
		Log("\t.Features = [0x%02x,0x%02x,0x%02x,0x%02x,0x%02x]",
			gMPFloatPtr->Features[0],	gMPFloatPtr->Features[1],
			gMPFloatPtr->Features[2],	gMPFloatPtr->Features[3],
			gMPFloatPtr->Features[4]
			);
		Log("}");
		
		mptable = (void*)( KERNEL_BASE|gMPFloatPtr->MPConfig );
		Log("mptable = %p", mptable);
		Log("*mptable = {");
		Log("\t.Sig = 0x%08x", mptable->Sig);
		Log("\t.BaseTableLength = 0x%04x", mptable->BaseTableLength);
		Log("\t.SpecRev = 0x%02x", mptable->SpecRev);
		Log("\t.Checksum = 0x%02x", mptable->Checksum);
		Log("\t.OEMID = '%8c'", mptable->OemID);
		Log("\t.ProductID = '%8c'", mptable->ProductID);
		Log("\t.OEMTablePtr = %p'", mptable->OEMTablePtr);
		Log("\t.OEMTableSize = 0x%04x", mptable->OEMTableSize);
		Log("\t.EntryCount = 0x%04x", mptable->EntryCount);
		Log("\t.LocalAPICMemMap = 0x%08x", mptable->LocalAPICMemMap);
		Log("\t.ExtendedTableLen = 0x%04x", mptable->ExtendedTableLen);
		Log("\t.ExtendedTableChecksum = 0x%02x", mptable->ExtendedTableChecksum);
		Log("}");
		
		gpMP_LocalAPIC = (void*)MM_MapHWPage(mptable->LocalAPICMemMap, 1);
		
		ents = mptable->Entries;
		giNumCPUs = 0;
		
		for( i = 0; i < mptable->EntryCount; i ++ )
		{
			 int	entSize = 0;
			switch( ents->Type )
			{
			case 0:	// Processor
				entSize = 20;
				Log("%i: Processor", i);
				Log("\t.APICID = %i", ents->Proc.APICID);
				Log("\t.APICVer = 0x%02x", ents->Proc.APICVer);
				Log("\t.CPUFlags = 0x%02x", ents->Proc.CPUFlags);
				Log("\t.CPUSignature = 0x%08x", ents->Proc.CPUSignature);
				Log("\t.FeatureFlags = 0x%08x", ents->Proc.FeatureFlags);
				
				
				if( !(ents->Proc.CPUFlags & 1) ) {
					Log("DISABLED");
					break;
				}
				
				// Check if there is too many processors
				if(giNumCPUs >= MAX_CPUS) {
					giNumCPUs ++;	// If `giNumCPUs` > MAX_CPUS later, it will be clipped
					break;
				}
				
				// Initialise CPU Info
				gaAPIC_to_CPU[ents->Proc.APICID] = giNumCPUs;
				gaCPUs[giNumCPUs].APICID = ents->Proc.APICID;
				gaCPUs[giNumCPUs].State = 0;
				giNumCPUs ++;
				
				// Send IPI
				if( !(ents->Proc.CPUFlags & 2) )
				{
					MP_StartAP( giNumCPUs-1 );
				}
				
				break;
			case 1:	// Bus
				entSize = 8;
				Log("%i: Bus", i);
				Log("\t.ID = %i", ents->Bus.ID);
				Log("\t.TypeString = '%6c'", ents->Bus.TypeString);
				break;
			case 2:	// I/O APIC
				entSize = 8;
				Log("%i: I/O APIC", i);
				Log("\t.ID = %i", ents->IOAPIC.ID);
				Log("\t.Version = 0x%02x", ents->IOAPIC.Version);
				Log("\t.Flags = 0x%02x", ents->IOAPIC.Flags);
				Log("\t.Addr = 0x%08x", ents->IOAPIC.Addr);
				break;
			case 3:	// I/O Interrupt Assignment
				entSize = 8;
				Log("%i: I/O Interrupt Assignment", i);
				Log("\t.IntType = %i", ents->IOInt.IntType);
				Log("\t.Flags = 0x%04x", ents->IOInt.Flags);
				Log("\t.SourceBusID = 0x%02x", ents->IOInt.SourceBusID);
				Log("\t.SourceBusIRQ = 0x%02x", ents->IOInt.SourceBusIRQ);
				Log("\t.DestAPICID = 0x%02x", ents->IOInt.DestAPICID);
				Log("\t.DestAPICIRQ = 0x%02x", ents->IOInt.DestAPICIRQ);
				break;
			case 4:	// Local Interrupt Assignment
				entSize = 8;
				Log("%i: Local Interrupt Assignment", i);
				Log("\t.IntType = %i", ents->LocalInt.IntType);
				Log("\t.Flags = 0x%04x", ents->LocalInt.Flags);
				Log("\t.SourceBusID = 0x%02x", ents->LocalInt.SourceBusID);
				Log("\t.SourceBusIRQ = 0x%02x", ents->LocalInt.SourceBusIRQ);
				Log("\t.DestLocalAPICID = 0x%02x", ents->LocalInt.DestLocalAPICID);
				Log("\t.DestLocalAPICIRQ = 0x%02x", ents->LocalInt.DestLocalAPICIRQ);
				break;
			default:
				Log("%i: Unknown (%i)", i, ents->Type);
				break;
			}
			ents = (void*)( (Uint)ents + entSize );
		}
		
		if( giNumCPUs > MAX_CPUS ) {
			Warning("Too many CPUs detected (%i), only using %i of them", giNumCPUs, MAX_CPUS);
			giNumCPUs = MAX_CPUS;
		}
	
		while( giNumInitingCPUs )
			MM_FinishVirtualInit();
		
		Panic("Uh oh... MP Table Parsing is unimplemented\n");
	}
	else {
		Log("No MP Table was found, assuming uniprocessor\n");
		giNumCPUs = 1;
		gTSSs = &gTSS0;
	}
	#else
	giNumCPUs = 1;
	gTSSs = &gTSS0;
	MM_FinishVirtualInit();
	#endif
	
	#if USE_MP
	// Initialise Normal TSS(s)
	for(pos=0;pos<giNumCPUs;pos++)
	{
	#else
	pos = 0;
	#endif
		gTSSs[pos].RSP0 = 0;	// Set properly by scheduler
		gGDT[7+pos*2].LimitLow = sizeof(tTSS) & 0xFFFF;
		gGDT[7+pos*2].BaseLow = ((Uint)(&gTSSs[pos])) & 0xFFFF;
		gGDT[7+pos*2].BaseMid = ((Uint)(&gTSSs[pos])) >> 16;
		gGDT[7+pos*2].BaseHi = ((Uint)(&gTSSs[pos])) >> 24;
		gGDT[7+pos*2+1].DWord[0] = ((Uint)(&gTSSs[pos])) >> 32;
	#if USE_MP
	}
	for(pos=0;pos<giNumCPUs;pos++) {
		__asm__ __volatile__ ("ltr %%ax"::"a"(0x38+pos*16));
	}
	#else
	__asm__ __volatile__ ("ltr %%ax"::"a"(0x38));
	#endif
	
	// Set Debug registers
	__asm__ __volatile__ ("mov %0, %%db0" : : "r"(&gThreadZero));
	__asm__ __volatile__ ("mov %%rax, %%db1" : : "a"(0));
	
	gaCPUs[0].Current = &gThreadZero;
	
	gProcessZero.MemState.CR3 = (Uint)gInitialPML4 - KERNEL_BASE;
	gThreadZero.CurCPU = 0;
	gThreadZero.KernelStack = 0xFFFFA00000000000 + KERNEL_STACK_SIZE;
	
	// Set timer frequency
	outb(0x43, 0x34);	// Set Channel 0, Low/High, Rate Generator
	outb(0x40, PIT_TIMER_DIVISOR&0xFF);	// Low Byte of Divisor
	outb(0x40, (PIT_TIMER_DIVISOR>>8)&0xFF);	// High Byte
	
	// Create Per-Process Data Block
	if( !MM_Allocate(MM_PPD_CFG) )
	{
		Warning("Oh, hell, Unable to allocate PPD for Thread#0");
	}

	Proc_InitialiseSSE();

	Log_Log("Proc", "Multithreading initialised");
}

#if USE_MP
void MP_StartAP(int CPU)
{
	Log("Starting AP %i (APIC %i)", CPU, gaCPUs[CPU].APICID);
	// Set location of AP startup code and mark for a warm restart
	*(Uint16*)(KERNEL_BASE|0x467) = (Uint)&APStartup - (KERNEL_BASE|0xFFFF0);
	*(Uint16*)(KERNEL_BASE|0x469) = 0xFFFF;
	outb(0x70, 0x0F);	outb(0x71, 0x0A);	// Warm Reset
	MP_SendIPI(gaCPUs[CPU].APICID, 0, 5);
	giNumInitingCPUs ++;
}

void MP_SendIPI(Uint8 APICID, int Vector, int DeliveryMode)
{
	Uint32	addr = (Uint)gpMP_LocalAPIC + 0x300;
	Uint32	val;
	
	// High
	val = (Uint)APICID << 24;
	Log("*%p = 0x%08x", addr+0x10, val);
	*(Uint32*)(addr+0x10) = val;
	// Low (and send)
	val = ((DeliveryMode & 7) << 8) | (Vector & 0xFF);
	Log("*%p = 0x%08x", addr, val);
	*(Uint32*)addr = val;
}
#endif

/**
 * \brief Idle task
 */
void Proc_IdleTask(void *ptr)
{
	tCPU	*cpu = ptr;
	cpu->IdleThread = Proc_GetCurThread();
	cpu->IdleThread->ThreadName = (char*)"Idle Thread";
	Threads_SetPriority( cpu->IdleThread, -1 );	// Never called randomly
	cpu->IdleThread->Quantum = 1;	// 1 slice quantum
	for(;;) {
		HALT();	// Just yeilds
		Threads_Yield();
	}
}

/**
 * \fn void Proc_Start(void)
 * \brief Start process scheduler
 */
void Proc_Start(void)
{
	#if USE_MP
	 int	i;
	#endif
	
	#if USE_MP
	// Start APs
	for( i = 0; i < giNumCPUs; i ++ )
	{
		 int	tid;
		if(i)	gaCPUs[i].Current = NULL;
		
		Proc_NewKThread(Proc_IdleTask, &gaCPUs[i]);		

		// Create Idle Task
		gaCPUs[i].IdleThread = Threads_GetThread(tid);
		
		
		// Start the AP
		if( i != giProc_BootProcessorID ) {
			MP_StartAP( i );
		}
	}
	
	// BSP still should run the current task
	gaCPUs[0].Current = &gThreadZero;
	__asm__ __volatile__ ("mov %0, %%db0" : : "r"(&gThreadZero));
	
	// Start interrupts and wait for APs to come up
	Log("Waiting for APs to come up\n");
	__asm__ __volatile__ ("sti");
	while( giNumInitingCPUs )	__asm__ __volatile__ ("hlt");
	#else
	Proc_NewKThread(Proc_IdleTask, &gaCPUs[0]);
	
	// Start Interrupts (and hence scheduler)
	__asm__ __volatile__("sti");
	#endif
	MM_FinishVirtualInit();
	Log_Log("Proc", "Multithreading started");
}

/**
 * \fn tThread *Proc_GetCurThread(void)
 * \brief Gets the current thread
 */
tThread *Proc_GetCurThread(void)
{
	#if USE_MP
	tThread	*ret;
	__asm__ __volatile__ ("mov %%db0, %0" : "=r"(thread));
	return ret;	// gaCPUs[ GetCPUNum() ].Current;
	#else
	return gaCPUs[ 0 ].Current;
	#endif
}

void Proc_ClearProcess(tProcess *Process)
{
	Log_Warning("Proc", "TODO: Nuke address space etc");
}

/*
 * 
 */
void Proc_ClearThread(tThread *Thread)
{
}

/**
 * \brief Create a new kernel thread
 */
tTID Proc_NewKThread(void (*Fcn)(void*), void *Data)
{
	Uint	rsp;
	tThread	*newThread, *cur;
	
	cur = Proc_GetCurThread();
	newThread = Threads_CloneTCB(0);
	if(!newThread)	return -1;
	
	// Create new KStack
	newThread->KernelStack = MM_NewKStack();
	// Check for errors
	if(newThread->KernelStack == 0) {
		free(newThread);
		return -1;
	}

	rsp = newThread->KernelStack;
	*(Uint*)(rsp-=8) = (Uint)Data;	// Data (shadowed)
	*(Uint*)(rsp-=8) = (Uint)Fcn;	// Function to call
	*(Uint*)(rsp-=8) = (Uint)newThread;	// Thread ID
	
	newThread->SavedState.RSP = rsp;
	newThread->SavedState.RIP = (Uint)&NewTaskHeader;
	newThread->SavedState.SSE = NULL;
//	Log("New (KThread) %p, rsp = %p\n", newThread->SavedState.RIP, newThread->SavedState.RSP);
	
//	MAGIC_BREAK();	
	Threads_AddActive(newThread);

	return newThread->TID;
}

/**
 * \fn int Proc_Clone(Uint Flags)
 * \brief Clone the current process
 */
tTID Proc_Clone(Uint Flags)
{
	tThread	*newThread, *cur = Proc_GetCurThread();
	Uint	rip;

	// Sanity check	
	if( !(Flags & CLONE_VM) ) {
		Log_Error("Proc", "Proc_Clone: Don't leave CLONE_VM unset, use Proc_NewKThread instead");
		return -1;
	}

	// Create new TCB
	newThread = Threads_CloneTCB(Flags);
	if(!newThread)	return -1;
	
	// Save core machine state
	rip = Proc_CloneInt(&newThread->SavedState.RSP, &newThread->Process->MemState.CR3);
	if(rip == 0)	return 0;	// Child
	newThread->KernelStack = cur->KernelStack;
	newThread->SavedState.RIP = rip;
	newThread->SavedState.SSE = NULL;

	// DEBUG
	#if 0
	Log("New (Clone) %p, rsp = %p, cr3 = %p", rip, newThread->SavedState.RSP, newThread->MemState.CR3);
	{
		Uint cr3;
		__asm__ __volatile__ ("mov %%cr3, %0" : "=r" (cr3));
		Log("Current CR3 = 0x%x, PADDR(RSP) = 0x%x", cr3, MM_GetPhysAddr(newThread->SavedState.RSP));
	}
	#endif
	// /DEBUG
	
	// Lock list and add to active
	Threads_AddActive(newThread);
	
	return newThread->TID;
}

/**
 * \fn int Proc_SpawnWorker(void)
 * \brief Spawns a new worker thread
 */
tThread *Proc_SpawnWorker(void (*Fcn)(void*), void *Data)
{
	tThread	*new, *cur;
	Uint	stack_contents[3];

	cur = Proc_GetCurThread();
	
	// Create new thread
	new = Threads_CloneThreadZero();
	if(!new) {
		Warning("Proc_SpawnWorker - Out of heap space!\n");
		return NULL;
	}

	// Create the stack contents
	stack_contents[2] = (Uint)Data;
	stack_contents[1] = (Uint)Fcn;
	stack_contents[0] = (Uint)new;
	
	// Create a new worker stack (in PID0's address space)
	// - The stack is built by this code using stack_contents
	new->KernelStack = MM_NewWorkerStack(stack_contents, sizeof(stack_contents));

	new->SavedState.RSP = new->KernelStack - sizeof(stack_contents);
	new->SavedState.RIP = (Uint)&NewTaskHeader;
	new->SavedState.SSE = NULL;
	
//	Log("New (Worker) %p, rsp = %p\n", new->SavedState.RIP, new->SavedState.RSP);
	
	// Mark as active
	new->Status = THREAD_STAT_PREINIT;
	Threads_AddActive( new );
	
	return new;
}

/**
 * \brief Creates a new user stack
 */
Uint Proc_MakeUserStack(void)
{
	 int	i;
	Uint	base = USER_STACK_TOP - USER_STACK_SZ;
	
	// Check Prospective Space
	for( i = USER_STACK_SZ >> 12; i--; )
	{
		if( MM_GetPhysAddr( (void*)(base + (i<<12)) ) != 0 )
			break;
	}
	
	if(i != -1)	return 0;
	
	// Allocate Stack - Allocate incrementally to clean up MM_Dump output
	// - Most of the user stack is the zero page
	for( i = 0; i < (USER_STACK_SZ-USER_STACK_PREALLOC)/0x1000; i++ )
	{
		MM_AllocateZero( base + (i<<12) );
	}
	// - but the top USER_STACK_PREALLOC pages are actually allocated
	for( ; i < USER_STACK_SZ/0x1000; i++ )
	{
		tPAddr	alloc = MM_Allocate( base + (i<<12) );
		if( !alloc )
		{
			// Error
			Log_Error("Proc", "Unable to allocate user stack (%i pages requested)", USER_STACK_SZ/0x1000);
			while( i -- )
				MM_Deallocate( base + (i<<12) );
			return 0;
		}
	}
	
	return base + USER_STACK_SZ;
}

void Proc_StartUser(Uint Entrypoint, Uint Base, int ArgC, const char **ArgV, int DataSize)
{
	Uint	*stack;
	 int	i;
	const char	**envp = NULL;
	Uint16	ss, cs;
	
	
	// Copy Arguments
	stack = (void*)Proc_MakeUserStack();
	if(!stack) {
		Log_Error("Proc", "Unable to create user stack!");
		Threads_Exit(0, -1);
	}
	stack -= (DataSize+7)/8;
	memcpy( stack, ArgV, DataSize );
	free(ArgV);
	
	// Adjust Arguments and environment
	if(DataSize)
	{
		Uint	delta = (Uint)stack - (Uint)ArgV;
		ArgV = (const char**)stack;
		for( i = 0; ArgV[i]; i++ )	ArgV[i] += delta;
		envp = &ArgV[i+1];
		for( i = 0; envp[i]; i++ )	envp[i] += delta;
	}
	
	// User Mode Segments
	// 0x2B = 64-bit
	ss = 0x23;	cs = 0x2B;
	
	// Arguments
	*--stack = (Uint)envp;
	*--stack = (Uint)ArgV;
	*--stack = (Uint)ArgC;
	*--stack = Base;
	
	Proc_StartProcess(ss, (Uint)stack, 0x202, cs, Entrypoint);
}

void Proc_StartProcess(Uint16 SS, Uint Stack, Uint Flags, Uint16 CS, Uint IP)
{
	if( !(CS == 0x1B || CS == 0x2B) || SS != 0x23 ) {
		Log_Error("Proc", "Proc_StartProcess: CS / SS are not valid (%x, %x)",
			CS, SS);
		Threads_Exit(0, -1);
	}
//	Log("Proc_StartProcess: (SS=%x, Stack=%p, Flags=%x, CS=%x, IP=%p)", SS, Stack, Flags, CS, IP);
//	MM_DumpTables(0, USER_MAX);
	if(CS == 0x1B)
	{
		// 32-bit return
		__asm__ __volatile__ (
			"mov %0, %%rsp;\n\t"	// Set stack pointer
			"mov %2, %%r11;\n\t"	// Set RFLAGS
			"sysret;\n\t"
			: : "r" (Stack), "c" (IP), "r" (Flags)
			);
	}
	else
	{
		// 64-bit return
		__asm__ __volatile__ (
			"mov %0, %%rsp;\n\t"	// Set stack pointer
			"mov %2, %%r11;\n\t"	// Set RFLAGS
			"sysretq;\n\t"
			: : "r" (Stack), "c" (IP), "r" (Flags)
			);
	}
	for(;;);
}

/**
 * \fn int Proc_Demote(Uint *Err, int Dest, tRegs *Regs)
 * \brief Demotes a process to a lower permission level
 * \param Err	Pointer to user's errno
 * \param Dest	New Permission Level
 * \param Regs	Pointer to user's register structure
 */
int Proc_Demote(Uint *Err, int Dest, tRegs *Regs)
{
	 int	cpl = Regs->CS & 3;
	// Sanity Check
	if(Dest > 3 || Dest < 0) {
		*Err = -EINVAL;
		return -1;
	}
	
	// Permission Check
	if(cpl > Dest) {
		*Err = -EACCES;
		return -1;
	}
	
	// Change the Segment Registers
	Regs->CS = (((Dest+1)<<4) | Dest) - 8;
	Regs->SS = ((Dest+1)<<4) | Dest;
	
	return 0;
}

/**
 * \brief Calls a signal handler in user mode
 * \note Used for signals
 */
void Proc_CallFaultHandler(tThread *Thread)
{
	// Never returns
	Proc_ReturnToUser(Thread->FaultHandler, Thread->KernelStack, Thread->CurFaultNum);
	for(;;);
}

void Proc_DumpThreadCPUState(tThread *Thread)
{
	Log("  At %04x:%016llx", Thread->SavedState.UserCS, Thread->SavedState.UserRIP);
}

void Proc_Reschedule(void)
{
	tThread	*nextthread, *curthread;
	 int	cpu = GetCPUNum();

	// TODO: Wait for it?
	if(IS_LOCKED(&glThreadListLock))        return;
	
	curthread = gaCPUs[cpu].Current;

	nextthread = Threads_GetNextToRun(cpu, curthread);

	if(nextthread == curthread)	return ;
	if(!nextthread)
		nextthread = gaCPUs[cpu].IdleThread;
	if(!nextthread)
		return ;

	#if DEBUG_TRACE_SWITCH
	LogF("\nSwitching to task CR3 = 0x%x, RIP = %p, RSP = %p - %i (%s)\n",
		nextthread->Process->MemState.CR3,
		nextthread->SavedState.RIP,
		nextthread->SavedState.RSP,
		nextthread->TID,
		nextthread->ThreadName
		);
	#endif

	// Update CPU state
	gaCPUs[cpu].Current = nextthread;
	gTSSs[cpu].RSP0 = nextthread->KernelStack-4;
	__asm__ __volatile__ ("mov %0, %%db0" : : "r" (nextthread));

	if( curthread )
	{
		// Save FPU/MMX/XMM/SSE state
		if( curthread->SavedState.SSE )
		{
			Proc_SaveSSE( ((Uint)curthread->SavedState.SSE + 0xF) & ~0xF );
			curthread->SavedState.bSSEModified = 0;
			Proc_DisableSSE();
		}
		SwitchTasks(
			nextthread->SavedState.RSP, &curthread->SavedState.RSP,
			nextthread->SavedState.RIP, &curthread->SavedState.RIP,
			nextthread->Process->MemState.CR3
			);
	}
	else
	{
		Uint	tmp;
		SwitchTasks(
			nextthread->SavedState.RSP, &tmp,
			nextthread->SavedState.RIP, &tmp,
			nextthread->Process->MemState.CR3
			);
	}
	return ;
}

/**
 * \fn void Proc_Scheduler(int CPU)
 * \brief Swap current thread and clears dead threads
 */
void Proc_Scheduler(int CPU, Uint RSP, Uint RIP)
{
#if 0
	tThread	*thread;

	// If the spinlock is set, let it complete
	if(IS_LOCKED(&glThreadListLock))	return;
	
	// Get current thread
	thread = gaCPUs[CPU].Current;

	if( thread )
	{
		tRegs	*regs;
		// Reduce remaining quantum and continue timeslice if non-zero
		if(thread->Remaining--)	return;
		// Reset quantum for next call
		thread->Remaining = thread->Quantum;
		
		// TODO: Make this more stable somehow
		{
			regs = (tRegs*)(RSP+(1)*8);	// CurThread
			thread->SavedState.UserCS = regs->CS;
			thread->SavedState.UserRIP = regs->RIP;
		}
	}

	// ACK Timer here?

	Proc_Reschedule();
#endif
}

// === EXPORTS ===
EXPORT(Proc_SpawnWorker);
