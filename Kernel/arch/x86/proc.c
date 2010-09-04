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

// === FLAGS ===
#define DEBUG_TRACE_SWITCH	0

// === CONSTANTS ===
#define	SWITCH_MAGIC	0xFF5317C8	// FF SWITCH - There is no code in this area
// Base is 1193182
#define TIMER_BASE      1193182
#define TIMER_DIVISOR   11932	//~100Hz

// === TYPES ===
#if USE_MP
typedef struct sCPU
{
	Uint8	APICID;
	Uint8	State;	// 0: Unavaliable, 1: Idle, 2: Active
	Uint16	Resvd;
	tThread	*Current;
	tThread	*IdleThread;
}	tCPU;
#endif

// === IMPORTS ===
extern tGDT	gGDT[];
extern tIDT	gIDT[];
extern void APWait(void);	// 16-bit AP pause code
extern void APStartup(void);	// 16-bit AP startup code
extern Uint	GetEIP(void);	// start.asm
extern int	GetCPUNum(void);	// start.asm
extern Uint32	gaInitPageDir[1024];	// start.asm
extern char	Kernel_Stack_Top[];
extern tShortSpinlock	glThreadListLock;
extern int	giNumCPUs;
extern int	giNextTID;
extern tThread	gThreadZero;
extern tThread	*Threads_CloneTCB(Uint *Err, Uint Flags);
extern void	Isr8(void);	// Double Fault
extern void	Proc_ReturnToUser(tVAddr Handler, Uint Argument);

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
volatile Uint32	giMP_TimerCount;	// Start Count for Local APIC Timer
tAPIC	*gpMP_LocalAPIC = NULL;
Uint8	gaAPIC_to_CPU[256] = {0};
tCPU	gaCPUs[MAX_CPUS];
tTSS	gaTSSs[MAX_CPUS];	// TSS Array
 int	giProc_BootProcessorID = 0;
#else
tThread	*gCurrentThread = NULL;
tThread	*gpIdleThread = NULL;
#endif
#if USE_PAE
Uint32	*gPML4s[4] = NULL;
#endif
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
	MM_FinishVirtualInit();
	#endif
	
	#if 0
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
	
	#if USE_MP
	gaCPUs[0].Current = &gThreadZero;
	#else
	gCurrentThread = &gThreadZero;
	#endif
	gThreadZero.CurCPU = 0;
	
	#if USE_PAE
	gThreadZero.MemState.PDP[0] = 0;
	gThreadZero.MemState.PDP[1] = 0;
	gThreadZero.MemState.PDP[2] = 0;
	#else
	gThreadZero.MemState.CR3 = (Uint)gaInitPageDir - KERNEL_BASE;
	#endif
	
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
		gpIdleThread = Proc_GetCurThread();
		gpIdleThread->ThreadName = "Idle Thread";
		gpIdleThread->NumTickets = 0;	// Never called randomly
		gpIdleThread->Quantum = 1;	// 1 slice quantum
		for(;;)	HALT();	// Just yeilds
	}
	
	// Set current task
	gCurrentThread = &gThreadZero;
	
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
	return gCurrentThread;
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

/**
 * \fn int Proc_Clone(Uint *Err, Uint Flags)
 * \brief Clone the current process
 */
int Proc_Clone(Uint *Err, Uint Flags)
{
	tThread	*newThread;
	tThread	*cur = Proc_GetCurThread();
	Uint	eip, esp, ebp;
	
	__asm__ __volatile__ ("mov %%esp, %0": "=r"(esp));
	__asm__ __volatile__ ("mov %%ebp, %0": "=r"(ebp));
	
	newThread = Threads_CloneTCB(Err, Flags);
	if(!newThread)	return -1;
	
	// Initialise Memory Space (New Addr space or kernel stack)
	if(Flags & CLONE_VM) {
		newThread->MemState.CR3 = MM_Clone();
		newThread->KernelStack = cur->KernelStack;
	} else {
		Uint	tmpEbp, oldEsp = esp;

		// Set CR3
		#if USE_PAE
		# warning "PAE Unimplemented"
		#else
		newThread->MemState.CR3 = cur->MemState.CR3;
		#endif

		// Create new KStack
		newThread->KernelStack = MM_NewKStack();
		// Check for errors
		if(newThread->KernelStack == 0) {
			free(newThread);
			return -1;
		}

		// Get ESP as a used size
		esp = cur->KernelStack - esp;
		// Copy used stack
		memcpy( (void*)(newThread->KernelStack - esp), (void*)(cur->KernelStack - esp), esp );
		// Get ESP as an offset in the new stack
		esp = newThread->KernelStack - esp;
		// Adjust EBP
		ebp = newThread->KernelStack - (cur->KernelStack - ebp);

		// Repair EBPs & Stack Addresses
		// Catches arguments also, but may trash stack-address-like values
		for(tmpEbp = esp; tmpEbp < newThread->KernelStack; tmpEbp += 4)
		{
			if(oldEsp < *(Uint*)tmpEbp && *(Uint*)tmpEbp < cur->KernelStack)
				*(Uint*)tmpEbp += newThread->KernelStack - cur->KernelStack;
		}
	}
	
	// Save core machine state
	newThread->SavedState.ESP = esp;
	newThread->SavedState.EBP = ebp;
	eip = GetEIP();
	if(eip == SWITCH_MAGIC) {
		__asm__ __volatile__ ("mov %0, %%db0" : : "r" (newThread) );
		#if USE_MP
		// ACK the interrupt
		if( GetCPUNum() )
			gpMP_LocalAPIC->EOI.Val = 0;
		else
		#endif
			outb(0x20, 0x20);	// ACK Timer and return as child
		__asm__ __volatile__ ("sti");	// Restart interrupts
		return 0;
	}
	
	// Set EIP as parent
	newThread->SavedState.EIP = eip;
	
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
	Uint	eip, esp, ebp;
	
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
	// - The stack is relocated by this function
	new->KernelStack = MM_NewWorkerStack();

	// Get ESP and EBP based in the new stack
	__asm__ __volatile__ ("mov %%esp, %0": "=r"(esp));
	__asm__ __volatile__ ("mov %%ebp, %0": "=r"(ebp));
	esp = new->KernelStack - (cur->KernelStack - esp);
	ebp = new->KernelStack - (cur->KernelStack - ebp);	
	
	// Save core machine state
	new->SavedState.ESP = esp;
	new->SavedState.EBP = ebp;
	eip = GetEIP();
	if(eip == SWITCH_MAGIC) {
		__asm__ __volatile__ ("mov %0, %%db0" : : "r"(new));
		#if USE_MP
		// ACK the interrupt
		if(GetCPUNum())
			gpMP_LocalAPIC->EOI.Val = 0;
		else
		#endif
			outb(0x20, 0x20);	// ACK Timer and return as child
		__asm__ __volatile__ ("sti");	// Restart interrupts
		return 0;
	}
	
	// Set EIP as parent
	new->SavedState.EIP = eip;
	// Mark as active
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
	
	//Log("stack = %p", stack);
	
	// Copy Arguments
	stack -= DataSize/sizeof(*stack);
	memcpy( stack, ArgV, DataSize );
	
	//Log("stack = %p", stack);
	
	if( DataSize )
	{
		// Adjust Arguments and environment
		delta = (Uint)stack - (Uint)ArgV;
		ArgV = (char**)stack;
		for( i = 0; ArgV[i]; i++ )
			ArgV[i] += delta;
		i ++;
		
		// Do we care about EnvP?
		if( EnvP ) {
			EnvP = &ArgV[i];
			for( i = 0; EnvP[i]; i++ )
				EnvP[i] += delta;
		}
	}
	
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
	Proc_ReturnToUser( Thread->FaultHandler, Thread->CurFaultNum );
	for(;;);
}

/**
 * \fn void Proc_Scheduler(int CPU)
 * \brief Swap current thread and clears dead threads
 */
void Proc_Scheduler(int CPU)
{
	Uint	esp, ebp, eip;
	tThread	*thread;
	
	// If the spinlock is set, let it complete
	if(IS_LOCKED(&glThreadListLock))	return;
	
	// Get current thread
	#if USE_MP
	thread = gaCPUs[CPU].Current;
	#else
	thread = gCurrentThread;
	#endif
	
	if( thread )
	{
		// Reduce remaining quantum and continue timeslice if non-zero
		if( thread->Remaining-- )
			return;
		// Reset quantum for next call
		thread->Remaining = thread->Quantum;
		
		// Get machine state
		__asm__ __volatile__ ( "mov %%esp, %0" : "=r" (esp) );
		__asm__ __volatile__ ( "mov %%ebp, %0" : "=r" (ebp) );
		eip = GetEIP();
		if(eip == SWITCH_MAGIC)	return;	// Check if a switch happened
		
		// Save machine state
		thread->SavedState.ESP = esp;
		thread->SavedState.EBP = ebp;
		thread->SavedState.EIP = eip;
	}
	
	// Get next thread to run
	thread = Threads_GetNextToRun(CPU, thread);
	
	// No avaliable tasks, just go into low power mode (idle thread)
	if(thread == NULL) {
		#if USE_MP
		thread = gaCPUs[CPU].IdleThread;
		Log("CPU %i Running Idle Thread", CPU);
		#else
		thread = gpIdleThread;
		#endif
	}
	
	// Set current thread
	#if USE_MP
	gaCPUs[CPU].Current = thread;
	#else
	gCurrentThread = thread;
	#endif
	
	#if DEBUG_TRACE_SWITCH
	Log("Switching to task %i, CR3 = 0x%x, EIP = %p",
		thread->TID,
		thread->MemState.CR3,
		thread->SavedState.EIP
		);
	#endif
	
	#if USE_MP	// MP Debug
	Log("CPU = %i, Thread %p", CPU, thread);
	#endif
	
	// Update Kernel Stack pointer
	gTSSs[CPU].ESP0 = thread->KernelStack-4;
	
	// Set address space
	#if USE_PAE
	# error "Todo: Implement PAE Address space switching"
	#else
	__asm__ __volatile__ ("mov %0, %%cr3" : : "a" (thread->MemState.CR3));
	#endif
	
	#if 0
	if(thread->SavedState.ESP > 0xC0000000
	&& thread->SavedState.ESP < thread->KernelStack-0x2000) {
		Log_Warning("Proc", "Possible bad ESP %p (PID %i)", thread->SavedState.ESP);
	}
	#endif
	
	// Switch threads
	__asm__ __volatile__ (
		"mov %1, %%esp\n\t"	// Restore ESP
		"mov %2, %%ebp\n\t"	// and EBP
		"jmp *%3" : :	// And return to where we saved state (Proc_Clone or Proc_Scheduler)
		"a"(SWITCH_MAGIC), "b"(thread->SavedState.ESP),
		"d"(thread->SavedState.EBP), "c"(thread->SavedState.EIP)
		);
	for(;;);	// Shouldn't reach here
}

// === EXPORTS ===
EXPORT(Proc_SpawnWorker);
