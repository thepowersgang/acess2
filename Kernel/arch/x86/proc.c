/*
 * AcessOS Microkernel Version
 * proc.c
 */
#include <acess.h>
#include <threads.h>
#include <proc.h>
#include <desctab.h>
#include <mm_virt.h>
#include <errno.h>
#if USE_MP
# include <mp.h>
#endif
#include <hal_proc.h>

// === FLAGS ===
#define DEBUG_TRACE_SWITCH	0
#define DEBUG_DISABLE_DOUBLEFAULT	1
#define DEBUG_VERY_SLOW_SWITCH	0

// === CONSTANTS ===
// Base is 1193182
#define TIMER_BASE      1193182
#if DEBUG_VERY_SLOW_PERIOD
# define TIMER_DIVISOR	1193	//~10Hz switch, with 10 quantum = 1s per thread
#else
# define TIMER_DIVISOR	11932	//~100Hz
#endif

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
extern tIDT	gIDT[];
extern void	APWait(void);	// 16-bit AP pause code
extern void	APStartup(void);	// 16-bit AP startup code
extern Uint	GetEIP(void);	// start.asm
extern Uint	GetEIP_Sched(void);	// proc.asm
extern void	NewTaskHeader(tThread *Thread, void *Fcn, int nArgs, ...);	// Actually takes cdecl args
extern Uint	Proc_CloneInt(Uint *ESP, Uint32 *CR3);
extern Uint32	gaInitPageDir[1024];	// start.asm
extern char	Kernel_Stack_Top[];
extern int	giNumCPUs;
extern int	giNextTID;
extern tThread	gThreadZero;
extern void	Isr8(void);	// Double Fault
extern void	Proc_ReturnToUser(tVAddr Handler, Uint Argument, tVAddr KernelStack);
extern char	scheduler_return[];	// Return address in SchedulerBase
extern char	IRQCommon[];	// Common IRQ handler code
extern char	IRQCommon_handled[];	// IRQCommon call return location
extern char	GetEIP_Sched_ret[];	// GetEIP call return location
extern void	Threads_AddToDelete(tThread *Thread);
extern void	SwitchTasks(Uint NewSP, Uint *OldSP, Uint NewIP, Uint *OldIO, Uint CR3);
extern void	Proc_InitialiseSSE(void);
extern void	Proc_SaveSSE(Uint DestPtr);
extern void	Proc_DisableSSE(void);

// === PROTOTYPES ===
//void	ArchThreads_Init(void);
#if USE_MP
void	MP_StartAP(int CPU);
void	MP_SendIPI(Uint8 APICID, int Vector, int DeliveryMode);
#endif
void	Proc_IdleThread(void *Ptr);
//void	Proc_Start(void);
//tThread	*Proc_GetCurThread(void);
void	Proc_ChangeStack(void);
// int	Proc_NewKThread(void (*Fcn)(void*), void *Data);
// int	Proc_Clone(Uint *Err, Uint Flags);
Uint	Proc_MakeUserStack(void);
//void	Proc_StartUser(Uint Entrypoint, Uint *Bases, int ArgC, char **ArgV, char **EnvP, int DataSize);
void	Proc_StartProcess(Uint16 SS, Uint Stack, Uint Flags, Uint16 CS, Uint IP) NORETURN;
 int	Proc_Demote(Uint *Err, int Dest, tRegs *Regs);
//void	Proc_CallFaultHandler(tThread *Thread);
//void	Proc_DumpThreadCPUState(tThread *Thread);
void	Proc_Scheduler(int CPU);

// === GLOBALS ===
// --- Multiprocessing ---
#if USE_MP
volatile int	giNumInitingCPUs = 0;
tMPInfo	*gMPFloatPtr = NULL;
volatile Uint32	giMP_TimerCount;	// Start Count for Local APIC Timer
tAPIC	*gpMP_LocalAPIC = NULL;
Uint8	gaAPIC_to_CPU[256] = {0};
 int	giProc_BootProcessorID = 0;
tTSS	gaTSSs[MAX_CPUS];	// TSS Array
#endif
tCPU	gaCPUs[MAX_CPUS];
tTSS	*gTSSs = NULL;	// Pointer to TSS array
tTSS	gTSS0 = {0};
// --- Error Recovery ---
char	gaDoubleFaultStack[1024] __attribute__ ((section(".padata")));
tTSS	gDoubleFault_TSS = {
	.ESP0 = (Uint)&gaDoubleFaultStack[1024],
	.SS0 = 0x10,
	.CR3 = (Uint)gaInitPageDir - KERNEL_BASE,
	.EIP = (Uint)Isr8,
	.ESP = (Uint)&gaDoubleFaultStack[1024],
	.CS = 0x08,	.SS = 0x10,
	.DS = 0x10,	.ES = 0x10,
	.FS = 0x10,	.GS = 0x10,
};

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
		
		gpMP_LocalAPIC = (void*)MM_MapHWPages(mptable->LocalAPICMemMap, 1);
		
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
				
				// Set BSP Variable
				if( ents->Proc.CPUFlags & 2 ) {
					giProc_BootProcessorID = giNumCPUs-1;
				}
				
				break;
			
			#if DUMP_MP_TABLES
			case 1:	// Bus
				entSize = 8;
				Log("%i: Bus", i);
				Log("\t.ID = %i", ents->Bus.ID);
				Log("\t.TypeString = '%6C'", ents->Bus.TypeString);
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
			#endif
			}
			ents = (void*)( (Uint)ents + entSize );
		}
		
		if( giNumCPUs > MAX_CPUS ) {
			Warning("Too many CPUs detected (%i), only using %i of them", giNumCPUs, MAX_CPUS);
			giNumCPUs = MAX_CPUS;
		}
		gTSSs = gaTSSs;
	}
	else {
		Log("No MP Table was found, assuming uniprocessor\n");
		giNumCPUs = 1;
		gTSSs = &gTSS0;
	}
	#else
	giNumCPUs = 1;
	gTSSs = &gTSS0;
	#endif
	
	#if !DEBUG_DISABLE_DOUBLEFAULT
	// Initialise Double Fault TSS
	gGDT[5].BaseLow = (Uint)&gDoubleFault_TSS & 0xFFFF;
	gGDT[5].BaseMid = (Uint)&gDoubleFault_TSS >> 16;
	gGDT[5].BaseHi = (Uint)&gDoubleFault_TSS >> 24;
	
	// Set double fault IDT to use the new TSS
	gIDT[8].OffsetLo = 0;
	gIDT[8].CS = 5<<3;
	gIDT[8].Flags = 0x8500;
	gIDT[8].OffsetHi = 0;
	#endif
	
	// Set timer frequency
	outb(0x43, 0x34);	// Set Channel 0, Low/High, Rate Generator
	outb(0x40, TIMER_DIVISOR&0xFF);	// Low Byte of Divisor
	outb(0x40, (TIMER_DIVISOR>>8)&0xFF);	// High Byte
	
	Log("Timer Frequency %i.%03i Hz",
		TIMER_BASE/TIMER_DIVISOR,
		((Uint64)TIMER_BASE*1000/TIMER_DIVISOR)%1000
		);
	
	#if USE_MP
	// Get the count setting for APIC timer
	Log("Determining APIC Count");
	__asm__ __volatile__ ("sti");
	while( giMP_TimerCount == 0 )	__asm__ __volatile__ ("hlt");
	__asm__ __volatile__ ("cli");
	Log("APIC Count %i", giMP_TimerCount);
	{
		Uint64	freq = giMP_TimerCount;
		freq /= TIMER_DIVISOR;
		freq *= TIMER_BASE;
		if( (freq /= 1000) < 2*1000)
			Log("Bus Frequency %i KHz", freq);
		else if( (freq /= 1000) < 2*1000)
			Log("Bus Frequency %i MHz", freq);
		else if( (freq /= 1000) < 2*1000)
			Log("Bus Frequency %i GHz", freq);
		else
			Log("Bus Frequency %i THz", freq);
	}
	
	// Initialise Normal TSS(s)
	for(pos=0;pos<giNumCPUs;pos++)
	{
	#else
	pos = 0;
	#endif
		gTSSs[pos].SS0 = 0x10;
		gTSSs[pos].ESP0 = 0;	// Set properly by scheduler
		gGDT[6+pos].BaseLow = ((Uint)(&gTSSs[pos])) & 0xFFFF;
		gGDT[6+pos].BaseMid = ((Uint)(&gTSSs[pos]) >> 16) & 0xFFFF;
		gGDT[6+pos].BaseHi = ((Uint)(&gTSSs[pos])) >> 24;
	#if USE_MP
	}
	#endif
	
	// Load the BSP's TSS
	__asm__ __volatile__ ("ltr %%ax"::"a"(0x30));
	// Set Current Thread and CPU Number in DR0 and DR1
	__asm__ __volatile__ ("mov %0, %%db0"::"r"(&gThreadZero));
	__asm__ __volatile__ ("mov %0, %%db1"::"r"(0));
	
	gaCPUs[0].Current = &gThreadZero;
	gThreadZero.CurCPU = 0;
	
	gThreadZero.MemState.CR3 = (Uint)gaInitPageDir - KERNEL_BASE;
	
	// Create Per-Process Data Block
	if( !MM_Allocate(MM_PPD_CFG) )
	{
		Panic("OOM - No space for initial Per-Process Config");
	}

	// Initialise SSE support
	Proc_InitialiseSSE();
	
	// Change Stacks
	Proc_ChangeStack();
}

#if USE_MP
void MP_StartAP(int CPU)
{
	Log("Starting AP %i (APIC %i)", CPU, gaCPUs[CPU].APICID);
	
	// Set location of AP startup code and mark for a warm restart
	*(Uint16*)(KERNEL_BASE|0x467) = (Uint)&APWait - (KERNEL_BASE|0xFFFF0);
	*(Uint16*)(KERNEL_BASE|0x469) = 0xFFFF;
	outb(0x70, 0x0F);	outb(0x71, 0x0A);	// Set warm reset flag
	MP_SendIPI(gaCPUs[CPU].APICID, 0, 5);	// Init IPI
	
	// Delay
	inb(0x80); inb(0x80); inb(0x80); inb(0x80);
	
	// TODO: Use a better address, preferably registered with the MM
	// - MM_AllocDMA mabye?
	// Create a far jump
	*(Uint8*)(KERNEL_BASE|0x11000) = 0xEA;	// Far JMP
	*(Uint16*)(KERNEL_BASE|0x11001) = (Uint)&APStartup - (KERNEL_BASE|0xFFFF0);	// IP
	*(Uint16*)(KERNEL_BASE|0x11003) = 0xFFFF;	// CS
	// Send a Startup-IPI to make the CPU execute at 0x11000 (which we
	// just filled)
	MP_SendIPI(gaCPUs[CPU].APICID, 0x11, 6);	// StartupIPI
	
	giNumInitingCPUs ++;
}

/**
 * \brief Send an Inter-Processor Interrupt
 * \param APICID	Processor's Local APIC ID
 * \param Vector	Argument of some kind
 * \param DeliveryMode	Type of signal?
 */
void MP_SendIPI(Uint8 APICID, int Vector, int DeliveryMode)
{
	Uint32	val;
	
	// Hi
	val = (Uint)APICID << 24;
	Log("*%p = 0x%08x", &gpMP_LocalAPIC->ICR[1], val);
	gpMP_LocalAPIC->ICR[1].Val = val;
	// Low (and send)
	val = ((DeliveryMode & 7) << 8) | (Vector & 0xFF);
	Log("*%p = 0x%08x", &gpMP_LocalAPIC->ICR[0], val);
	gpMP_LocalAPIC->ICR[0].Val = val;
}
#endif

void Proc_IdleThread(void *Ptr)
{
	tCPU	*cpu = Ptr;
	cpu->IdleThread->ThreadName = strdup("Idle Thread");
	Threads_SetPriority( cpu->IdleThread, -1 );	// Never called randomly
	cpu->IdleThread->Quantum = 1;	// 1 slice quantum
	for(;;) {
		HALT();
		Proc_Reschedule();
	}
}

/**
 * \fn void Proc_Start(void)
 * \brief Start process scheduler
 */
void Proc_Start(void)
{
	 int	tid;
	#if USE_MP
	 int	i;
	#endif
	
	#if USE_MP
	// Start APs
	for( i = 0; i < giNumCPUs; i ++ )
	{
		if(i)	gaCPUs[i].Current = NULL;
		
		// Create Idle Task
		tid = Proc_NewKThread(Proc_IdleThread, &gaCPUs[i]);
		gaCPUs[i].IdleThread = Threads_GetThread(tid);
		
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
	tid = Proc_NewKThread(Proc_IdleThread, &gaCPUs[0]);
	gaCPUs[0].IdleThread = Threads_GetThread(tid);
	
	// Set current task
	gaCPUs[0].Current = &gThreadZero;

//	while( gaCPUs[0].IdleThread == NULL )
//		Threads_Yield();
	
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
	Uint	esp, ebp;
	Uint	tmpEbp, oldEsp;
	Uint	curBase, newBase;

	__asm__ __volatile__ ("mov %%esp, %0":"=r"(esp));
	__asm__ __volatile__ ("mov %%ebp, %0":"=r"(ebp));

	oldEsp = esp;

	// Create new KStack
	newBase = MM_NewKStack();
	// Check for errors
	if(newBase == 0) {
		Panic("What the?? Unable to allocate space for initial kernel stack");
		return;
	}

	curBase = (Uint)&Kernel_Stack_Top;
	
	LOG("curBase = 0x%x, newBase = 0x%x", curBase, newBase);

	// Get ESP as a used size
	esp = curBase - esp;
	LOG("memcpy( %p, %p, 0x%x )", (void*)(newBase - esp), (void*)(curBase - esp), esp );
	// Copy used stack
	memcpy( (void*)(newBase - esp), (void*)(curBase - esp), esp );
	// Get ESP as an offset in the new stack
	esp = newBase - esp;
	// Adjust EBP
	ebp = newBase - (curBase - ebp);

	// Repair EBPs & Stack Addresses
	// Catches arguments also, but may trash stack-address-like values
	for(tmpEbp = esp; tmpEbp < newBase; tmpEbp += 4)
	{
		if(oldEsp < *(Uint*)tmpEbp && *(Uint*)tmpEbp < curBase)
			*(Uint*)tmpEbp += newBase - curBase;
	}
	
	Proc_GetCurThread()->KernelStack = newBase;
	
	__asm__ __volatile__ ("mov %0, %%esp"::"r"(esp));
	__asm__ __volatile__ ("mov %0, %%ebp"::"r"(ebp));
}

void Proc_ClearThread(tThread *Thread)
{
	if(Thread->SavedState.SSE) {
		free(Thread->SavedState.SSE);
		Thread->SavedState.SSE = NULL;
	}
}

int Proc_NewKThread(void (*Fcn)(void*), void *Data)
{
	Uint	esp;
	tThread	*newThread, *cur;
	
	cur = Proc_GetCurThread();
	newThread = Threads_CloneTCB(0);
	if(!newThread)	return -1;
	
	// Set CR3
	newThread->MemState.CR3 = cur->MemState.CR3;

	// Create new KStack
	newThread->KernelStack = MM_NewKStack();
	// Check for errors
	if(newThread->KernelStack == 0) {
		free(newThread);
		return -1;
	}

	esp = newThread->KernelStack;
	*(Uint*)(esp-=4) = (Uint)Data;	// Data (shadowed)
	*(Uint*)(esp-=4) = 1;	// Number of params
	*(Uint*)(esp-=4) = (Uint)Fcn;	// Function to call
	*(Uint*)(esp-=4) = (Uint)newThread;	// Thread ID
	
	newThread->SavedState.ESP = esp;
	newThread->SavedState.EIP = (Uint)&NewTaskHeader;
	newThread->SavedState.SSE = NULL;
	Log("New (KThread) %p, esp = %p\n", newThread->SavedState.EIP, newThread->SavedState.ESP);
	
//	MAGIC_BREAK();	
	Threads_AddActive(newThread);

	return newThread->TID;
}

/**
 * \fn int Proc_Clone(Uint *Err, Uint Flags)
 * \brief Clone the current process
 */
int Proc_Clone(Uint Flags)
{
	tThread	*newThread;
	tThread	*cur = Proc_GetCurThread();
	Uint	eip;

	// Sanity, please
	if( !(Flags & CLONE_VM) ) {
		Log_Error("Proc", "Proc_Clone: Don't leave CLONE_VM unset, use Proc_NewKThread instead");
		return -1;
	}
	
	// New thread
	newThread = Threads_CloneTCB(Flags);
	if(!newThread)	return -1;

	newThread->KernelStack = cur->KernelStack;

	// Clone state
	eip = Proc_CloneInt(&newThread->SavedState.ESP, &newThread->MemState.CR3);
	if( eip == 0 ) {
		// ACK the interrupt
		return 0;
	}
	newThread->SavedState.EIP = eip;
	newThread->SavedState.SSE = NULL;
	newThread->SavedState.bSSEModified = 0;
	
	// Check for errors
	if( newThread->MemState.CR3 == 0 ) {
		Log_Error("Proc", "Proc_Clone: MM_Clone failed");
		Threads_AddToDelete(newThread);
		return -1;
	}

	// Add the new thread to the run queue
	Threads_AddActive(newThread);
	return newThread->TID;
}

/**
 * \fn int Proc_SpawnWorker(void)
 * \brief Spawns a new worker thread
 */
int Proc_SpawnWorker(void (*Fcn)(void*), void *Data)
{
	tThread	*new;
	Uint	stack_contents[4];
	
	// Create new thread
	new = Threads_CloneThreadZero();
	if(!new) {
		Warning("Proc_SpawnWorker - Out of heap space!\n");
		return -1;
	}

	// Create the stack contents
	stack_contents[3] = (Uint)Data;
	stack_contents[2] = 1;
	stack_contents[1] = (Uint)Fcn;
	stack_contents[0] = (Uint)new;
	
	// Create a new worker stack (in PID0's address space)
	new->KernelStack = MM_NewWorkerStack(stack_contents, sizeof(stack_contents));

	// Save core machine state
	new->SavedState.ESP = new->KernelStack - sizeof(stack_contents);
	new->SavedState.EIP = (Uint)NewTaskHeader;
	new->SavedState.SSE = NULL;
	new->SavedState.bSSEModified = 0;
	
	// Mark as active
	new->Status = THREAD_STAT_PREINIT;
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
	for( i = 0; i < USER_STACK_SZ/0x1000; i++ )
	{
		if( !MM_Allocate( base + (i<<12) ) )
		{
			Warning("OOM: Proc_MakeUserStack");
			return 0;
		}
	}
	
	return base + USER_STACK_SZ;
}

void Proc_StartUser(Uint Entrypoint, Uint Base, int ArgC, char **ArgV, int DataSize)
{
	Uint	*stack;
	 int	i;
	char	**envp = NULL;
	Uint16	ss, cs;
	
	// Copy data to the user stack and free original buffer
	stack = (void*)Proc_MakeUserStack();
	stack -= DataSize/sizeof(*stack);
	memcpy( stack, ArgV, DataSize );
	free(ArgV);
	
	// Adjust Arguments and environment
	if( DataSize )
	{
		Uint delta = (Uint)stack - (Uint)ArgV;
		ArgV = (char**)stack;
		for( i = 0; ArgV[i]; i++ )	ArgV[i] += delta;
		envp = &ArgV[i+1];
		for( i = 0; envp[i]; i++ )	envp[i] += delta;
	}
	
	// User Mode Segments
	ss = 0x23;	cs = 0x1B;
	
	// Arguments
	*--stack = (Uint)envp;
	*--stack = (Uint)ArgV;
	*--stack = (Uint)ArgC;
	*--stack = Base;
	
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
	*--stack = 0x54545454;	// esp - NOT POPED
	*--stack = 0x51515151;	// esi
	*--stack = 0xB4B4B4B4;	// ebp
	//Individual PUSHs
	*--stack = SS;	// ds
	*--stack = SS;	// es
	*--stack = SS;	// fs
	*--stack = SS;	// gs
	
	__asm__ __volatile__ (
	"mov %%eax,%%esp;\n\t"	// Set stack pointer
	"pop %%gs;\n\t"
	"pop %%fs;\n\t"
	"pop %%es;\n\t"
	"pop %%ds;\n\t"
	"popa;\n\t"
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
	 int	cpl = Regs->cs & 3;
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
	Regs->cs = (((Dest+1)<<4) | Dest) - 8;
	Regs->ss = ((Dest+1)<<4) | Dest;
	// Check if the GP Segs are GDT, then change them
	if(!(Regs->ds & 4))	Regs->ds = ((Dest+1)<<4) | Dest;
	if(!(Regs->es & 4))	Regs->es = ((Dest+1)<<4) | Dest;
	if(!(Regs->fs & 4))	Regs->fs = ((Dest+1)<<4) | Dest;
	if(!(Regs->gs & 4))	Regs->gs = ((Dest+1)<<4) | Dest;
	
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
	Proc_ReturnToUser( Thread->FaultHandler, Thread->CurFaultNum, Thread->KernelStack );
	for(;;);
}

void Proc_DumpThreadCPUState(tThread *Thread)
{
	if( Thread->CurCPU > -1 )
	{
		 int	maxBacktraceDistance = 6;
		tRegs	*regs = NULL;
		Uint32	*stack;
		
		if( Thread->CurCPU != GetCPUNum() ) {
			Log("  Currently running");
			return ;
		}
		
		// Backtrace to find the IRQ entrypoint
		// - This will usually only be called by an IRQ, so this should
		//   work
		__asm__ __volatile__ ("mov %%ebp, %0" : "=r" (stack));
		while( maxBacktraceDistance -- )
		{
			// [ebp] = oldEbp
			// [ebp+4] = retaddr
			
			if( stack[1] == (tVAddr)&IRQCommon_handled ) {
				regs = (void*)stack[2];
				break;
			}
			
			stack = (void*)stack[0];
		}
		
		if( !regs ) {
			Log("  Unable to find IRQ Entry");
			return ;
		}
		
		Log("  at %04x:%08x", regs->cs, regs->eip);
		return ;
	}
	
	tVAddr	diffFromScheduler = Thread->SavedState.EIP - (tVAddr)SwitchTasks;
	tVAddr	diffFromClone = Thread->SavedState.EIP - (tVAddr)Proc_CloneInt;
	tVAddr	diffFromSpawn = Thread->SavedState.EIP - (tVAddr)NewTaskHeader;
	
	if( diffFromClone > 0 && diffFromClone < 40 )	// When I last checked, .newTask was at .+27
	{
		Log("  Creating process");
		return ;
	}
	
	if( diffFromSpawn == 0 )
	{
		Log("  Creating thread");
		return ;
	}
	
	if( diffFromScheduler > 0 && diffFromScheduler < 128 )	// When I last checked, GetEIP was at .+0x30
	{
		// Scheduled out
		Log("  At %04x:%08x", Thread->SavedState.UserCS, Thread->SavedState.UserEIP);
		return ;
	}
	
	Log("  Just created (unknown %p)", Thread->SavedState.EIP);
}

void Proc_Reschedule(void)
{
	tThread	*nextthread, *curthread;
	 int	cpu = GetCPUNum();

	// TODO: Wait for the lock?
	if(IS_LOCKED(&glThreadListLock))        return;
	
	curthread = Proc_GetCurThread();

	nextthread = Threads_GetNextToRun(cpu, curthread);

	if(!nextthread)
		nextthread = gaCPUs[cpu].IdleThread;
	if(!nextthread || nextthread == curthread)
		return ;

	#if DEBUG_TRACE_SWITCH
	LogF("\nSwitching to task %i, CR3 = 0x%x, EIP = %p, ESP = %p\n",
		nextthread->TID,
		nextthread->MemState.CR3,
		nextthread->SavedState.EIP,
		nextthread->SavedState.ESP
		);
	#endif

	// Update CPU state
	gaCPUs[cpu].Current = nextthread;
	gTSSs[cpu].ESP0 = nextthread->KernelStack-4;
	__asm__ __volatile__("mov %0, %%db0\n\t" : : "r"(nextthread) );

	// Save FPU/MMX/XMM/SSE state
	if( curthread->SavedState.SSE )
	{
		Proc_SaveSSE( ((Uint)curthread->SavedState.SSE + 0xF) & ~0xF );
		curthread->SavedState.bSSEModified = 0;
		Proc_DisableSSE();
	}

	SwitchTasks(
		nextthread->SavedState.ESP, &curthread->SavedState.ESP,
		nextthread->SavedState.EIP, &curthread->SavedState.EIP,
		nextthread->MemState.CR3
		);

	return ;
}

/**
 * \fn void Proc_Scheduler(int CPU)
 * \brief Swap current thread and clears dead threads
 */
void Proc_Scheduler(int CPU)
{
	tThread	*thread;
	
	// If the spinlock is set, let it complete
	if(IS_LOCKED(&glThreadListLock))	return;
	
	// Get current thread
	thread = gaCPUs[CPU].Current;
	
	if( thread )
	{
		tRegs	*regs;
		Uint	ebp;
		// Reduce remaining quantum and continue timeslice if non-zero
		if( thread->Remaining-- )
			return;
		// Reset quantum for next call
		thread->Remaining = thread->Quantum;
		
		// TODO: Make this more stable somehow
		__asm__ __volatile__("mov %%ebp, %0" : "=r" (ebp));
		regs = (tRegs*)(ebp+(2+2)*4);	// EBP,Ret + CPU,CurThread
		thread->SavedState.UserCS = regs->cs;
		thread->SavedState.UserEIP = regs->eip;
		
		if(thread->bInstrTrace) {
			regs->eflags |= 0x100;	// Set TF
			Log("%p De-scheduled", thread);
		}
		else
			regs->eflags &= ~0x100;	// Clear TF
	}

#if 0
	// TODO: Ack timer?
	#if USE_MP
	if( GetCPUNum() )
		gpMP_LocalAPIC->EOI.Val = 0;
	else
	#endif
		outb(0x20, 0x20);
	__asm__ __volatile__ ("sti");
	Proc_Reschedule();
#endif
}

// === EXPORTS ===
EXPORT(Proc_SpawnWorker);
