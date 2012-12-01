/*
 * Acess2 Kernel (x86 Core)
 * - By John Hodge (thePowersGang)
 *
 * acpica.c
 * - ACPICA Interface
 */
#include <acpi.h>
#include <timers.h>
#include <mutex.h>
#include <semaphore.h>

// === PROTOTYPES ===
int	ACPICA_Initialise(void);
void	ACPI_int_InterruptProxy(int IRQ, void *data);


// === CODE ===
int ACPICA_Initialise(void)
{
	ACPI_STATUS	rv;

	rv = AcpiInitializeSubsystem();
	if( ACPI_FAILURE(rv) )
	{
		Log_Error("ACPI", "AcpiInitializeSubsystem: %i", rv);
		return -1;
	}
	
	rv = AcpiInitializeTables(NULL, 16, FALSE);
	if( ACPI_FAILURE(rv) )
	{
		Log_Error("ACPI", "AcpiInitializeTables: %i", rv);
		return -1;
	}

	// AcpiInitializeTables?
	rv = AcpiLoadTables();
	if( ACPI_FAILURE(rv) )
	{
		Log_Error("ACPI", "AcpiLoadTables: %i", rv);
		return -1;
	}
	
	rv = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if( ACPI_FAILURE(rv) )
	{
		Log_Error("ACPI", "AcpiEnableSubsystem: %i", rv);
		return -1;
	}

	return 0;
}

// ---------------
// --- Exports ---
// ---------------
ACPI_STATUS AcpiOsInitialize(void)
{
	return AE_OK;
}

ACPI_STATUS AcpiOsTerminate(void)
{
	return AE_OK;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
{
	ACPI_SIZE	val;
	ACPI_STATUS	rv;
	
	rv = AcpiFindRootPointer(&val);
	if( ACPI_FAILURE(rv) )
		return 0;

	return val;
	// (Or use EFI)
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue)
{
	UNIMPLEMENTED();
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExisitingTable, ACPI_TABLE_HEADER **NewTable)
{
	UNIMPLEMENTED();
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExisitingTable, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength)
{
	UNIMPLEMENTED();
	return AE_NOT_IMPLEMENTED;
}

// -- Memory Management ---
ACPI_STATUS AcpiOsCreateCache(char *CacheName, UINT16 ObjectSize, UINT16 MaxDepth, ACPI_CACHE_T **ReturnCache)
{
	UNIMPLEMENTED();
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsDeleteCache(ACPI_CACHE_T *Cache)
{
	if( Cache == NULL )
		return AE_BAD_PARAMETER;
	
	UNIMPLEMENTED();
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsPurgeCache(ACPI_CACHE_T *Cache)
{
	if( Cache == NULL )
		return AE_BAD_PARAMETER;
	
	UNIMPLEMENTED();
	return AE_NOT_IMPLEMENTED;
}

void *AcpiOsAcquireObject(ACPI_CACHE_T *Cache)
{
	// TODO
	return NULL;
}

ACPI_STATUS AcpiOsReleaseObject(ACPI_CACHE_T *Cache, void *Object)
{
	if( Cache == NULL || Object == NULL )
		return AE_BAD_PARAMETER;
	UNIMPLEMENTED();
	return AE_NOT_IMPLEMENTED;
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length)
{
	Uint	ofs = PhysicalAddress & (PAGE_SIZE-1);
	int npages = (ofs + Length + (PAGE_SIZE-1)) / PAGE_SIZE;
	return (char*)MM_MapHWPages(PhysicalAddress, npages) + ofs;
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Length)
{
	Uint	ofs = (tVAddr)LogicalAddress & (PAGE_SIZE-1);
	int npages = (ofs + Length + (PAGE_SIZE-1)) / PAGE_SIZE;
	// TODO: Validate `Length` is the same as was passed to AcpiOsMapMemory
	MM_UnmapHWPages( (tVAddr)LogicalAddress, npages);
}

ACPI_STATUS AcpiOsGetPhysicalAddress(void *LogicalAddress, ACPI_PHYSICAL_ADDRESS *PhysicalAddress)
{
	if( LogicalAddress == NULL || PhysicalAddress == NULL )
		return AE_BAD_PARAMETER;
	
	tPAddr	rv = MM_GetPhysAddr(LogicalAddress);
	if( rv == 0 )
		return AE_ERROR;
	*PhysicalAddress = rv;
	return AE_OK;
}

void *AcpiOsAllocate(ACPI_SIZE Size)
{
	return malloc(Size);
}

void AcpiOsFree(void *Memory)
{
	return free(Memory);
}

BOOLEAN AcpiOsReadable(void *Memory, ACPI_SIZE Length)
{
	return CheckMem(Memory, Length);
}

BOOLEAN AcpiOsWritable(void *Memory, ACPI_SIZE Length)
{
	// TODO: Actually check if it's writable
	return CheckMem(Memory, Length);
}


// --- Threads ---
ACPI_THREAD_ID AcpiOsGetThreadId(void)
{
	return Threads_GetTID();
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE Type, ACPI_OSD_EXEC_CALLBACK Function, void *Context)
{
	// TODO: Need to store currently executing functions
	if( Function == NULL )
		return AE_BAD_PARAMETER;
	Proc_SpawnWorker(Function, Context);
	return AE_OK;
}

void AcpiOsSleep(UINT64 Milliseconds)
{
	Time_Delay(Milliseconds);
}

void AcpiOsStall(UINT32 Microseconds)
{
	// TODO: need a microsleep function
	Microseconds += (1000-1);
	Microseconds /= 1000;
	Time_Delay(Microseconds);
}

void AcpiOsWaitEventsComplete(void)
{
	// TODO: 
}

// --- Mutexes etc ---
ACPI_STATUS AcpiOsCreateMutex(ACPI_MUTEX *OutHandle)
{
	if( !OutHandle )
		return AE_BAD_PARAMETER;
	tMutex	*ret = calloc( sizeof(tMutex), 1 );
	if( !ret )
		return AE_NO_MEMORY;
	ret->Name = "AcpiOsCreateMutex";
	*OutHandle = ret;
	
	return AE_OK;
}

void AcpiOsDeleteMutex(ACPI_MUTEX Handle)
{
	Mutex_Acquire(Handle);
	free(Handle);
}

ACPI_STATUS AcpiOsAcquireMutex(ACPI_MUTEX Handle, UINT16 Timeout)
{
	if( Handle == NULL )
		return AE_BAD_PARAMETER;

	Mutex_Acquire(Handle);	

	return AE_OK;
}

void AcpiOsReleaseMutex(ACPI_MUTEX Handle)
{
	Mutex_Release(Handle);
}

ACPI_STATUS AcpiOsCreateSemaphore(UINT32 MaxUnits, UINT32 InitialUnits, ACPI_SEMAPHORE *OutHandle)
{
	if( !OutHandle )
		return AE_BAD_PARAMETER;
	tSemaphore	*ret = calloc( sizeof(tSemaphore), 1 );
	if( !ret )
		return AE_NO_MEMORY;
	
	Semaphore_Init(ret, InitialUnits, MaxUnits, "AcpiOsCreateSemaphore", "");
	*OutHandle = ret;
	return AE_OK;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE Handle)
{
	if( !Handle )
		return AE_BAD_PARAMETER;

	free(Handle);	

	return AE_OK;
}

ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units, UINT16 Timeout)
{
	if( !Handle )
		return AE_BAD_PARAMETER;

	// Special case
	if( Timeout == 0 )
	{
		// NOTE: Possible race condition
		if( Semaphore_GetValue(Handle) >= Units ) {
			Semaphore_Wait(Handle, Units);
			return AE_OK;
		}
		return AE_TIME;
	}

	tTime	start = now();
	UINT32	rem = Units;
	while(rem && now() - start < Timeout)
	{
		rem -= Semaphore_Wait(Handle, rem);
	}

	if( rem ) {
		Semaphore_Signal(Handle, Units - rem);
		return AE_TIME;
	}

	return AE_OK;
}

ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE Handle, UINT32 Units)
{
	if( !Handle )
		return AE_BAD_PARAMETER;

	// TODO: Support AE_LIMIT detection early (to avoid blocks)

	// NOTE: Blocks
	int rv = Semaphore_Signal(Handle, Units);
	if( rv != Units )
		return AE_LIMIT;
	
	return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *OutHandle)
{
	if( !OutHandle )
		return AE_BAD_PARAMETER;
	tShortSpinlock	*lock = calloc(sizeof(tShortSpinlock), 1);
	if( !lock )
		return AE_NO_MEMORY;
	
	*OutHandle = lock;
	return AE_OK;
}

void AcpiOsDeleteLock(ACPI_SPINLOCK Handle)
{
	free(Handle);
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK Handle)
{
	SHORTLOCK(Handle);
	return 0;
}

void AcpiOsReleaseLock(ACPI_SPINLOCK Handle, ACPI_CPU_FLAGS Flags)
{
	SHORTREL(Handle);
}

// --- Interrupt handling ---
#define N_INT_LEVELS	16
ACPI_OSD_HANDLER	gaACPI_InterruptHandlers[N_INT_LEVELS];
void	*gaACPI_InterruptData[N_INT_LEVELS];
 int	gaACPI_InterruptHandles[N_INT_LEVELS];

void ACPI_int_InterruptProxy(int IRQ, void *data)
{
	if( !gaACPI_InterruptHandlers[IRQ] )
		return ;
	gaACPI_InterruptHandlers[IRQ](gaACPI_InterruptData[IRQ]);
}

ACPI_STATUS AcpiOsInstallInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context)
{
	if( InterruptLevel >= N_INT_LEVELS || Handler == NULL )
		return AE_BAD_PARAMETER;
	if( gaACPI_InterruptHandlers[InterruptLevel] )
		return AE_ALREADY_EXISTS;

	gaACPI_InterruptHandlers[InterruptLevel] = Handler;
	gaACPI_InterruptData[InterruptLevel] = Context;

	gaACPI_InterruptHandles[InterruptLevel] = IRQ_AddHandler(InterruptLevel, ACPI_int_InterruptProxy, NULL);
	return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(UINT32 InterruptLevel, ACPI_OSD_HANDLER Handler)
{
	if( InterruptLevel >= N_INT_LEVELS || Handler == NULL )
		return AE_BAD_PARAMETER;
	if( gaACPI_InterruptHandlers[InterruptLevel] != Handler )
		return AE_NOT_EXIST;
	gaACPI_InterruptHandlers[InterruptLevel] = NULL;
	IRQ_RemHandler(gaACPI_InterruptHandles[InterruptLevel]);
	return AE_OK;
}

// --- Memory Access ---
ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 *Value, UINT32 Width)
{
	void *ptr;
	if( Address < 1024*1024 ) {
		ptr = (void*)(KERNEL_BASE | Address);
	}
	else {
		ptr = (char*)MM_MapTemp(Address) + (Address & 0xFFF);
	}

	switch(Width)
	{
	case 8: 	*Value = *(Uint8 *)ptr;	break;
	case 16:	*Value = *(Uint16*)ptr;	break;
	case 32:	*Value = *(Uint32*)ptr;	break;
	case 64:	*Value = *(Uint64*)ptr;	break;
	}

	if( Address >= 1024*1024 ) {
		MM_FreeTemp(ptr);
	}

	return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
	void *ptr;
	if( Address < 1024*1024 ) {
		ptr = (void*)(KERNEL_BASE | Address);
	}
	else {
		ptr = (char*)MM_MapTemp(Address) + (Address & 0xFFF);
	}

	switch(Width)
	{
	case 8: 	*(Uint8 *)ptr = Value;	break;
	case 16:	*(Uint16*)ptr = Value;	break;
	case 32:	*(Uint32*)ptr = Value;	break;
	case 64:	*(Uint64*)ptr = Value;	break;
	default:
		return AE_BAD_PARAMETER;
	}

	if( Address >= 1024*1024 ) {
		MM_FreeTemp(ptr);
	}
	
	return AE_OK;
}

// --- Port Input / Output ---
ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS Address, UINT32 *Value, UINT32 Width)
{
	switch(Width)
	{
	case 8: 	*Value = inb(Address);	break;
	case 16:	*Value = inw(Address);	break;
	case 32:	*Value = ind(Address);	break;
	default:
		return AE_BAD_PARAMETER;
	}
	return AE_OK;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS Address, UINT32 Value, UINT32 Width)
{
	switch(Width)
	{
	case 8: 	outb(Address, Value);	break;
	case 16:	outw(Address, Value);	break;
	case 32:	outd(Address, Value);	break;
	default:
		return AE_BAD_PARAMETER;
	}
	return AE_OK;
}

// --- PCI Configuration Space Access ---
ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 *Value, UINT32 Width)
{
	UNIMPLEMENTED();
	return AE_NOT_IMPLEMENTED;
}

ACPI_STATUS AcpiOsWritePciConfiguration(ACPI_PCI_ID *PciId, UINT32 Register, UINT64 Value, UINT32 Width)
{
	UNIMPLEMENTED();
	return AE_NOT_IMPLEMENTED;
}

// --- Formatted Output ---
void AcpiOsPrintf(const char *Format, ...)
{
	va_list	args;
	va_start(args, Format);

	LogV(Format, args);

	va_end(args);
}

void AcpiOsVprintf(const char *Format, va_list Args)
{
	LogV(Format, Args);
}

void AcpiOsRedirectOutput(void *Destination)
{
	// TODO: is this needed?
}

// --- Miscellaneous ---
UINT64 AcpiOsGetTimer(void)
{
	return now() * 10 * 1000;
}

ACPI_STATUS AcpiOsSignal(UINT32 Function, void *Info)
{
	switch(Function)
	{
	case ACPI_SIGNAL_FATAL: {
		ACPI_SIGNAL_FATAL_INFO	*finfo = Info;
		Log_Error("ACPI AML", "Fatal %x %x %x", finfo->Type, finfo->Code, finfo->Argument);
		break; }
	case ACPI_SIGNAL_BREAKPOINT: {
		Log_Notice("ACPI AML", "Breakpoint %s", Info);
		break; };
	}
	return AE_OK;
}

ACPI_STATUS AcpiOsGetLine(char *Buffer, UINT32 BufferLength, UINT32 *BytesRead)
{
	UNIMPLEMENTED();
	return AE_NOT_IMPLEMENTED;
}

