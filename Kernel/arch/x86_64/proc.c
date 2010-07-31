/*
 * Acess2 x86_64 port
 * proc.c
 */
#include <acess.h>
#include <proc.h>
#include <threads.h>
#include <desctab.h>
#include <mm_virt.h>
#include <errno.h>
#if USE_MP
# include <mp.h>
#endif

// === FLAGS ===
#define DEBUG_TRACE_SWITCH	0

// === CONSTANTS ===
#define	SWITCH_MAGIC	0x55ECAFFF##FFFACE55	// There is no code in this area
// Base is 1193182
#define TIMER_DIVISOR	11931	//~100Hz

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
extern Uint64	gInitialPML4[512];	// start.asm
extern char	gInitialKernelStack[];
extern tSpinlock	glThreadListLock;
extern int	giNumCPUs;
extern int	giNextTID;
extern int	giTotalTickets;
extern int	giNumActiveThreads;
extern tThread	gThreadZero;
//extern tThread	*Threads_GetNextToRun(int CPU);
extern void	Threads_Dump(void);
extern tThread	*Threads_CloneTCB(Uint *Err, Uint Flags);
extern void	Proc_ReturnToUser(void);
extern int	GetCPUNum(void);

// === PROTOTYPES ===
void	ArchThreads_Init(void);
#if USE_MP
void	MP_StartAP(int CPU);
void	MP_SendIPI(Uint8 APICID, int Vector, int DeliveryMode);
#endif
void	Proc_Start(void);
tThread	*Proc_GetCurThread(void);
void	Proc_ChangeStack(void);
 int	Proc_Clone(Uint *Err, Uint Flags);
void	Proc_StartProcess(Uint16 SS, Uint Stack, Uint Flags, Uint16 CS, Uint IP);
void	Proc_CallFaultHandler(tThread *Thread);
void	Proc_Scheduler(int CPU);

// === GLOBALS ===
// --- Multiprocessing ---
#if USE_MP
volatile int	giNumInitingCPUs = 0;
tMPInfo	*gMPFloatPtr = NULL;
tAPIC	*gpMP_LocalAPIC = NULL;
Uint8	gaAPIC_to_CPU[256] = {0};
#endif
tCPU	gaCPUs[MAX_CPUS];
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
	#endif
		__asm__ __volatile__ ("ltr %%ax"::"a"(0x38+pos*16));
	#if USE_MP
	}
	#endif
	
	gaCPUs[0].Current = &gThreadZero;
	
	gThreadZero.MemState.CR3 = (Uint)gInitialPML4 - KERNEL_BASE;
	
	// Set timer frequency
	outb(0x43, 0x34);	// Set Channel 0, Low/High, Rate Generator
	outb(0x40, TIMER_DIVISOR&0xFF);	// Low Byte of Divisor
	outb(0x40, (TIMER_DIVISOR>>8)&0xFF);	// High Byte
	
	// Create Per-Process Data Block
	MM_Allocate(MM_PPD_CFG);
	
	// Change Stacks
	Proc_ChangeStack();
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
		
		// Create Idle Task
		if( (tid = Proc_Clone(0, 0)) == 0)
		{
			for(;;)	HALT();	// Just yeilds
		}
		gaCPUs[i].IdleThread = Threads_GetThread(tid);
		gaCPUs[i].IdleThread->ThreadName = "Idle Thread";
		Threads_SetTickets( gaCPUs[i].IdleThread, 0 );	// Never called randomly
		gaCPUs[i].IdleThread->Quantum = 1;	// 1 slice quantum
		
		
		// Start the AP
		if( i != giProc_BootProcessorID ) {
			MP_StartAP( i );
		}
	}
	
	// BSP still should run the current task
	gaCPUs[0].Current = &gThreadZero;
	
	// Start interrupts and wait for APs to come up
	Log("Waiting for APs to come up\n");
	__asm__ __volatile__ ("sti");
	while( giNumInitingCPUs )	__asm__ __volatile__ ("hlt");
	#else
	// Create Idle Task
	if(Proc_Clone(0, 0) == 0)
	{
		gaCPUs[0].IdleThread = Proc_GetCurThread();
		gaCPUs[0].IdleThread->ThreadName = "Idle Thread";
		gaCPUs[0].IdleThread->NumTickets = 0;	// Never called randomly
		gaCPUs[0].IdleThread->Quantum = 1;	// 1 slice quantum
		for(;;)	HALT();	// Just yeilds
	}
	
	// Set current task
	gaCPUs[0].Current = &gThreadZero;
	
	// Start Interrupts (and hence scheduler)
	__asm__ __volatile__("sti");
	#endif
	MM_FinishVirtualInit();
}

/**
 * \fn tThread *Proc_GetCurThread(void)
 * \brief Gets the current thread
 */
tThread *Proc_GetCurThread(void)
{
	#if USE_MP
	return gaCPUs[ GetCPUNum() ].Current;
	#else
	return gaCPUs[ 0 ].Current;
	#endif
}

/**
 * \fn void Proc_ChangeStack(void)
 * \brief Swaps the current stack for a new one (in the proper stack reigon)
 */
void Proc_ChangeStack(void)
{
	Uint	rsp, rbp;
	Uint	tmp_rbp, old_rsp;
	Uint	curBase, newBase;

	__asm__ __volatile__ ("mov %%rsp, %0":"=r"(rsp));
	__asm__ __volatile__ ("mov %%rbp, %0":"=r"(rbp));

	old_rsp = rsp;

	// Create new KStack
	newBase = MM_NewKStack();
	// Check for errors
	if(newBase == 0) {
		Panic("What the?? Unable to allocate space for initial kernel stack");
		return;
	}

	curBase = (Uint)&gInitialKernelStack;
	
	Log("curBase = 0x%x, newBase = 0x%x", curBase, newBase);

	// Get ESP as a used size
	rsp = curBase - rsp;
	Log("memcpy( %p, %p, 0x%x )", (void*)(newBase - rsp), (void*)(curBase - rsp), rsp );
	// Copy used stack
	memcpy( (void*)(newBase - rsp), (void*)(curBase - rsp), rsp );
	// Get ESP as an offset in the new stack
	rsp = newBase - rsp;
	// Adjust EBP
	rbp = newBase - (curBase - rbp);

	Log("Update stack");
	// Repair EBPs & Stack Addresses
	// Catches arguments also, but may trash stack-address-like values
	for(tmp_rbp = rsp; tmp_rbp < newBase; tmp_rbp += sizeof(Uint))
	{
		if(old_rsp < *(Uint*)tmp_rbp && *(Uint*)tmp_rbp < curBase)
			*(Uint*)tmp_rbp += newBase - curBase;
	}
	
	Log("Applying Changes");
	Proc_GetCurThread()->KernelStack = newBase;
	__asm__ __volatile__ ("mov %0, %%rsp"::"r"(rsp));
	__asm__ __volatile__ ("mov %0, %%rbp"::"r"(rbp));
}

/**
 * \fn int Proc_Clone(Uint *Err, Uint Flags)
 * \brief Clone the current process
 */
int Proc_Clone(Uint *Err, Uint Flags)
{
	tThread	*newThread;
	tThread	*cur = Proc_GetCurThread();
	Uint	rip, rsp, rbp;
	
	__asm__ __volatile__ ("mov %%rsp, %0": "=r"(rsp));
	__asm__ __volatile__ ("mov %%rbp, %0": "=r"(rbp));
	
	newThread = Threads_CloneTCB(Err, Flags);
	if(!newThread)	return -1;
	
	Log("Proc_Clone: newThread = %p", newThread);
	
	// Initialise Memory Space (New Addr space or kernel stack)
	if(Flags & CLONE_VM) {
		Log("Proc_Clone: Cloning VM");
		newThread->MemState.CR3 = MM_Clone();
		newThread->KernelStack = cur->KernelStack;
	} else {
		Uint	tmp_rbp, old_rsp = rsp;

		// Set CR3
		newThread->MemState.CR3 = cur->MemState.CR3;

		// Create new KStack
		newThread->KernelStack = MM_NewKStack();
		Log("Proc_Clone: newKStack = %p", newThread->KernelStack);
		// Check for errors
		if(newThread->KernelStack == 0) {
			free(newThread);
			return -1;
		}

		// Get ESP as a used size
		rsp = cur->KernelStack - rsp;
		// Copy used stack
		memcpy(
			(void*)(newThread->KernelStack - rsp),
			(void*)(cur->KernelStack - rsp),
			rsp
			);
		// Get ESP as an offset in the new stack
		rsp = newThread->KernelStack - rsp;
		// Adjust EBP
		rbp = newThread->KernelStack - (cur->KernelStack - rbp);

		// Repair EBPs & Stack Addresses
		// Catches arguments also, but may trash stack-address-like values
		for(tmp_rbp = rsp; tmp_rbp < newThread->KernelStack; tmp_rbp += sizeof(Uint))
		{
			if(old_rsp < *(Uint*)tmp_rbp && *(Uint*)tmp_rbp < cur->KernelStack)
				*(Uint*)tmp_rbp += newThread->KernelStack - cur->KernelStack;
		}
	}
	
	// Save core machine state
	newThread->SavedState.RSP = rsp;
	newThread->SavedState.RBP = rbp;
	rip = GetRIP();
	if(rip == SWITCH_MAGIC) {
		outb(0x20, 0x20);	// ACK Timer and return as child
		return 0;
	}
	
	// Set EIP as parent
	newThread->SavedState.RIP = rip;
	
	// Lock list and add to active
	Threads_AddActive(newThread);
	
	return newThread->TID;
}

/**
 * \fn int Proc_SpawnWorker(void)
 * \brief Spawns a new worker thread
 */
int Proc_SpawnWorker(void)
{
	tThread	*new, *cur;
	Uint	rip, rsp, rbp;
	
	cur = Proc_GetCurThread();
	
	// Create new thread
	new = malloc( sizeof(tThread) );
	if(!new) {
		Warning("Proc_SpawnWorker - Out of heap space!\n");
		return -1;
	}
	memcpy(new, &gThreadZero, sizeof(tThread));
	// Set Thread ID
	new->TID = giNextTID++;
	// Create a new worker stack (in PID0's address space)
	// The stack is relocated by this code
	new->KernelStack = MM_NewWorkerStack();

	// Get ESP and EBP based in the new stack
	__asm__ __volatile__ ("mov %%rsp, %0": "=r"(rsp));
	__asm__ __volatile__ ("mov %%rbp, %0": "=r"(rbp));
	rsp = new->KernelStack - (cur->KernelStack - rsp);
	rbp = new->KernelStack - (cur->KernelStack - rbp);	
	
	// Save core machine state
	new->SavedState.RSP = rsp;
	new->SavedState.RBP = rbp;
	rip = GetRIP();
	if(rip == SWITCH_MAGIC) {
		outb(0x20, 0x20);	// ACK Timer and return as child
		return 0;
	}
	
	// Set EIP as parent
	new->SavedState.RIP = rip;
	// Mark as active
	new->Status = THREAD_STAT_ACTIVE;
	Threads_AddActive( new );
	
	return new->TID;
}

/**
 * \fn Uint Proc_MakeUserStack(void)
 * \brief Creates a new user stack
 */
Uint Proc_MakeUserStack(void)
{
	 int	i;
	Uint	base = USER_STACK_TOP - USER_STACK_SZ;
	
	// Check Prospective Space
	for( i = USER_STACK_SZ >> 12; i--; )
		if( MM_GetPhysAddr( base + (i<<12) ) != 0 )
			break;
	
	if(i != -1)	return 0;
	
	// Allocate Stack - Allocate incrementally to clean up MM_Dump output
	for( i = 0; i < USER_STACK_SZ/4069; i++ )
		MM_Allocate( base + (i<<12) );
	
	return base + USER_STACK_SZ;
}


/**
 * \fn void Proc_StartUser(Uint Entrypoint, Uint *Bases, int ArgC, char **ArgV, char **EnvP, int DataSize)
 * \brief Starts a user task
 */
void Proc_StartUser(Uint Entrypoint, Uint *Bases, int ArgC, char **ArgV, char **EnvP, int DataSize)
{
	Uint	*stack = (void*)Proc_MakeUserStack();
	 int	i;
	Uint	delta;
	Uint16	ss, cs;
	
	LOG("stack = 0x%x", stack);
	
	// Copy Arguments
	stack = (void*)( (Uint)stack - DataSize );
	memcpy( stack, ArgV, DataSize );
	
	// Adjust Arguments and environment
	delta = (Uint)stack - (Uint)ArgV;
	ArgV = (char**)stack;
	for( i = 0; ArgV[i]; i++ )	ArgV[i] += delta;
	i ++;
	EnvP = &ArgV[i];
	for( i = 0; EnvP[i]; i++ )	EnvP[i] += delta;
	
	// User Mode Segments
	ss = 0x23;	cs = 0x1B;
	
	// Arguments
	*--stack = (Uint)EnvP;
	*--stack = (Uint)ArgV;
	*--stack = (Uint)ArgC;
	while(*Bases)
		*--stack = *Bases++;
	*--stack = 0;	// Return Address
	
	Proc_StartProcess(ss, (Uint)stack, 0x202, cs, Entrypoint);
}

void Proc_StartProcess(Uint16 SS, Uint Stack, Uint Flags, Uint16 CS, Uint IP)
{
	Uint	*stack = (void*)Stack;
	*--stack = SS;		//Stack Segment
	*--stack = Stack;	//Stack Pointer
	*--stack = Flags;	//EFLAGS (Resvd (0x2) and IF (0x20))
	*--stack = CS;		//Code Segment
	*--stack = IP;	//EIP
	//PUSHAD
	*--stack = 0xAAAAAAAA;	// eax
	*--stack = 0xCCCCCCCC;	// ecx
	*--stack = 0xDDDDDDDD;	// edx
	*--stack = 0xBBBBBBBB;	// ebx
	*--stack = 0xD1D1D1D1;	// edi
	*--stack = 0x54545454;	// rsp - NOT POPED
	*--stack = 0x51515151;	// esi
	*--stack = 0xB4B4B4B4;	// rbp
	//Individual PUSHs
	*--stack = SS;	// ds
	
	__asm__ __volatile__ (
	"mov %%rax,%%rsp;\n\t"	// Set stack pointer
	"iret;\n\t" : : "a" (stack));
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
	// Rewinds the stack and calls the user function
	// Never returns
	__asm__ __volatile__ ("mov %0, %%rbp;\n\tcall Proc_ReturnToUser" :: "r"(Thread->FaultHandler));
	for(;;);
}

/**
 * \fn void Proc_Scheduler(int CPU)
 * \brief Swap current thread and clears dead threads
 */
void Proc_Scheduler(int CPU)
{
	Uint	rsp, rbp, rip;
	tThread	*thread;
	
	// If the spinlock is set, let it complete
	if(IS_LOCKED(&glThreadListLock))	return;
		
	// Get current thread
	thread = gaCPUs[CPU].Current;
	
	// Reduce remaining quantum and continue timeslice if non-zero
	if(thread->Remaining--)	return;
	// Reset quantum for next call
	thread->Remaining = thread->Quantum;
	
	// Get machine state
	__asm__ __volatile__ ("mov %%rsp, %0":"=r"(rsp));
	__asm__ __volatile__ ("mov %%rbp, %0":"=r"(rbp));
	rip = GetRIP();
	if(rip == SWITCH_MAGIC)	return;	// Check if a switch happened
	
	// Save machine state
	thread->SavedState.RSP = rsp;
	thread->SavedState.RBP = rbp;
	thread->SavedState.RIP = rip;
	
	// Get next thread
	thread = Threads_GetNextToRun(CPU, thread);
	
	// Error Check
	if(thread == NULL) {
		thread = gaCPUs[CPU].IdleThread;
		Warning("Hmm... Threads_GetNextToRun returned NULL, I don't think this should happen.\n");
		return;
	}
	
	#if DEBUG_TRACE_SWITCH
	Log("Switching to task %i, CR3 = 0x%x, RIP = %p",
		thread->TID,
		thread->MemState.CR3,
		thread->SavedState.RIP
		);
	#endif
	
	// Set current thread
	gaCPUs[CPU].Current = thread;
	
	// Update Kernel Stack pointer
	gTSSs[CPU].RSP0 = thread->KernelStack-4;
	
	// Set address space
	__asm__ __volatile__ ("mov %0, %%cr3"::"a"(thread->MemState.CR3));
	
	// Switch threads
	__asm__ __volatile__ (
		"mov %1, %%rsp\n\t"	// Restore RSP
		"mov %2, %%rbp\n\t"	// and RBP
		"jmp *%3" : :	// And return to where we saved state (Proc_Clone or Proc_Scheduler)
		"a"(SWITCH_MAGIC), "b"(thread->SavedState.RSP),
		"d"(thread->SavedState.RBP), "c"(thread->SavedState.RIP)
		);
	for(;;);	// Shouldn't reach here
}

// === EXPORTS ===
EXPORT(Proc_SpawnWorker);
