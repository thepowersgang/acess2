/*
 * Acess2 Kernel (x86 Core)
 * - By John Hodge (thePowersGang)
 *
 * acpica.c
 * - ACPICA Interface
 */
#define ACPI_DEBUG_OUTPUT	0
#define DEBUG	0
#define _AcpiModuleName "Shim"
#define _COMPONENT	"Acess"
#include <acpi.h>
#include <timers.h>
#include <mutex.h>
#include <semaphore.h>

#define ONEMEG	(1024*1024)

// === GLOBALS ===
// - RSDP Address from uEFI
tPAddr	gACPI_RSDPOverride = 0;

// === PROTOTYPES ===
int	ACPICA_Initialise(void);
void	ACPI_int_InterruptProxy(int IRQ, void *data);


// === CODE ===
int ACPICA_Initialise(void)
{
	ACPI_STATUS	rv;

	#ifdef ACPI_DEBUG_OUTPUT
	AcpiDbgLevel = ACPI_DB_ALL;
	#endif

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
		AcpiTerminate();
		return -1;
	}

	rv = AcpiLoadTables();
	if( ACPI_FAILURE(rv) )
	{
		Log_Error("ACPI", "AcpiLoadTables: %i", rv);
		AcpiTerminate();
		return -1;
	}
	
	rv = AcpiEnableSubsystem(ACPI_FULL_INITIALIZATION);
	if( ACPI_FAILURE(rv) )
	{
		Log_Error("ACPI", "AcpiEnableSubsystem: %i", rv);
		AcpiTerminate();
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

	if( gACPI_RSDPOverride )
		return gACPI_RSDPOverride;	

	rv = AcpiFindRootPointer(&val);
	if( ACPI_FAILURE(rv) )
		return 0;

	LOG("val=0x%x", val);
	
	return val;
	// (Or use EFI)
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *PredefinedObject, ACPI_STRING *NewValue)
{
	*NewValue = NULL;
	return AE_OK;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExisitingTable, ACPI_TABLE_HEADER **NewTable)
{
	*NewTable = NULL;
	return AE_OK;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExisitingTable, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength)
{
	*NewAddress = 0;
	return AE_OK;
}

// -- Memory Management ---
struct sACPICache
{
	Uint16	nObj;
	Uint16	ObjectSize;
	char	*Name;
	void	*First;
	char	ObjectStates[];
};

ACPI_STATUS AcpiOsCreateCache(char *CacheName, UINT16 ObjectSize, UINT16 MaxDepth, ACPI_CACHE_T **ReturnCache)
{
	tACPICache	*ret;
	 int	namelen = (CacheName ? strlen(CacheName) : 0) + 1;
	LOG("CacheName=%s, ObjSize=%x, MaxDepth=%x", CacheName, ObjectSize, MaxDepth);

	if( ReturnCache == NULL || ObjectSize < 16) {
		return AE_BAD_PARAMETER;
	}

	namelen = (namelen + 3) & ~3;

	ret = malloc(sizeof(*ret) + MaxDepth*sizeof(char) + namelen + MaxDepth*ObjectSize);
	if( !ret ) {
		Log_Notice("ACPICA", "%s: malloc() fail", __func__);
		return AE_NO_MEMORY;
	}

	ret->nObj = MaxDepth;
	ret->ObjectSize = ObjectSize;
	ret->Name = (char*)(ret->ObjectStates + MaxDepth);
	ret->First = ret->Name + namelen;
	if( CacheName )
		strcpy(ret->Name, CacheName);
	else
		ret->Name[0] = 0;
	memset(ret->ObjectStates, 0, sizeof(char)*MaxDepth);
	
	LOG("Allocated cache %p '%s' (%i x 0x%x)", ret, CacheName, MaxDepth, ObjectSize);
	
	*ReturnCache = ret;
	
	return AE_OK;
}

ACPI_STATUS AcpiOsDeleteCache(ACPI_CACHE_T *Cache)
{
	if( Cache == NULL )
		return AE_BAD_PARAMETER;

	free(Cache);
	return AE_OK;
}

ACPI_STATUS AcpiOsPurgeCache(ACPI_CACHE_T *Cache)
{
	ENTER("pCache", Cache);
	if( Cache == NULL ) {
		LEAVE('i', AE_BAD_PARAMETER);
		return AE_BAD_PARAMETER;
	}

	memset(Cache->ObjectStates, 0, sizeof(char)*Cache->nObj);

	LEAVE('i', AE_OK);
	return AE_OK;
}

void *AcpiOsAcquireObject(ACPI_CACHE_T *Cache)
{
	ENTER("pCache", Cache);
	LOG("Called by %p", __builtin_return_address(0));
	for(int i = 0; i < Cache->nObj; i ++ )
	{
		if( !Cache->ObjectStates[i] ) {
			Cache->ObjectStates[i] = 1;
			void *rv = (char*)Cache->First + i*Cache->ObjectSize;
			if(!rv) {
				LEAVE('n');
				return NULL;
			}
			memset(rv, 0, Cache->ObjectSize);
			LEAVE('p', rv);
			return rv;
		}
	}

	Log_Debug("ACPICA", "AcpiOsAcquireObject: All %i objects used in '%s'",
		Cache->nObj, Cache->Name);

	LEAVE('n');
	return NULL;
}

ACPI_STATUS AcpiOsReleaseObject(ACPI_CACHE_T *Cache, void *Object)
{
	if( Cache == NULL || Object == NULL )
		return AE_BAD_PARAMETER;
	ENTER("pCache pObject", Cache, Object);

	tVAddr delta = (tVAddr)Object - (tVAddr)Cache->First;
	delta /= Cache->ObjectSize;
	LOG("Cache=%p, delta = %i, (limit %i)", Cache, delta, Cache->nObj);
	
	if( delta >= Cache->nObj ) {
		LEAVE('i', AE_BAD_PARAMETER);
		return AE_BAD_PARAMETER;
	}
	
	Cache->ObjectStates[delta] = 0;

	LEAVE('i', AE_OK);
	return AE_OK;
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS PhysicalAddress, ACPI_SIZE Length)
{
	if( PhysicalAddress < ONEMEG )
		return (void*)(KERNEL_BASE | PhysicalAddress);
	
	Uint	ofs = PhysicalAddress & (PAGE_SIZE-1);
	int npages = (ofs + Length + (PAGE_SIZE-1)) / PAGE_SIZE;
	char *maploc = (void*)MM_MapHWPages(PhysicalAddress, npages);
	if(!maploc) {
		LOG("Mapping %P+0x%x failed", PhysicalAddress, Length);
		return NULL;
	}
//	MM_DumpTables(0, -1);
	void *rv = maploc + ofs;
	LOG("Map (%P+%i pg) to %p", PhysicalAddress, npages, rv);
	return rv;
}

void AcpiOsUnmapMemory(void *LogicalAddress, ACPI_SIZE Length)
{
	if( (tVAddr)LogicalAddress - KERNEL_BASE < ONEMEG )
		return ;

	LOG("%p", LogicalAddress);

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
	return Threads_GetTID() + 1;
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
	LOG("()");
	if( !OutHandle )
		return AE_BAD_PARAMETER;
	tMutex	*ret = calloc( sizeof(tMutex), 1 );
	if( !ret ) {
		Log_Notice("ACPICA", "%s: malloc() fail", __func__);
		return AE_NO_MEMORY;
	}
	ret->Name = "AcpiOsCreateMutex";
	*OutHandle = ret;
	
	return AE_OK;
}

void AcpiOsDeleteMutex(ACPI_MUTEX Handle)
{
	//  TODO: Need `Mutex_Destroy`
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
	LOG("(MaxUnits=%i,InitialUnits=%i)", MaxUnits, InitialUnits);
	if( !OutHandle )
		return AE_BAD_PARAMETER;
	tSemaphore	*ret = calloc( sizeof(tSemaphore), 1 );
	if( !ret ) {
		Log_Notice("ACPICA", "%s: malloc() fail", __func__);
		return AE_NO_MEMORY;
	}
	
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
	LOG("()");
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
	if( Address < ONEMEG ) {
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

	if( Address >= ONEMEG ) {
		MM_FreeTemp(ptr);
	}

	LOG("*%P = [%i]%X", Address, Width, *Value);

	return AE_OK;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, UINT64 Value, UINT32 Width)
{
	void *ptr;
	if( Address < ONEMEG ) {
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

	if( Address >= ONEMEG ) {
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

	LogFV(Format, args);

	va_end(args);
}

void AcpiOsVprintf(const char *Format, va_list Args)
{
	LogFV(Format, Args);
}

void AcpiOsRedirectOutput(void *Destination)
{
	// TODO: Do I even need to impliment this?
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

