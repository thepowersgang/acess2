/*
 * AcessOS Microkernel Version
 * mm_virt.c
 * 
 * Memory Map
 * 0xE0 - Kernel Base
 * 0xF0 - Kernel Stacks
 * 0xFD - Fractals
 * 0xFE - Unused
 * 0xFF - System Calls / Kernel's User Code
 */
#define DEBUG	0
#define SANITY	1
#include <acess.h>
#include <mm_virt.h>
#include <mm_phys.h>
#include <proc.h>
#include <hal_proc.h>
#include <arch_int.h>
#include <semaphore.h>

#include "include/vmem_layout.h"

#define TRACE_MAPS	0

#define TAB	22

#define	PF_PRESENT	0x1
#define	PF_WRITE	0x2
#define	PF_USER		0x4
#define PF_GLOBAL	0x80
#define	PF_COW		0x200
#define	PF_NOPAGE	0x400

#define INVLPG(addr)	__asm__ __volatile__ ("invlpg (%0)"::"r"(addr))

#define GET_TEMP_MAPPING(cr3) do { \
	__ASM__("cli"); \
	__AtomicTestSetLoop( (Uint *)gpTmpCR3, cr3 | 3 ); \
} while(0)
#define REL_TEMP_MAPPING() do { \
	*gpTmpCR3 = 0; \
	__ASM__("sti"); \
} while(0)

typedef Uint32	tTabEnt;

// === IMPORTS ===
extern tPage	_UsertextEnd;
extern tPage	_UsertextBase;
extern tPage	gKernelEnd;	// defined as page aligned
extern Uint32	gaInitPageDir[1024];
extern Uint32	gaInitPageTable[1024];
extern void	Threads_SegFault(tVAddr Addr);

// === PROTOTYPES ===
void	MM_PreinitVirtual(void);
void	MM_InstallVirtual(void);
void	MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs);
void	MM_DumpTables_Print(tVAddr Start, Uint32 Orig, size_t Size, void *Node);
//void	MM_DumpTables(tVAddr Start, tVAddr End);
//void	MM_ClearUser(void);
tPAddr	MM_DuplicatePage(tVAddr VAddr);

// === GLOBALS ===
#define gaPageTable	((tTabEnt*)PAGE_TABLE_ADDR)
#define gaPageDir	((tTabEnt*)PAGE_DIR_ADDR)
#define gaTmpTable	((tTabEnt*)TMP_TABLE_ADDR)
#define gaTmpDir	((tTabEnt*)TMP_DIR_ADDR)
#define gpPageCR3	((tTabEnt*)PAGE_CR3_ADDR)
#define gpTmpCR3	((tTabEnt*)TMP_CR3_ADDR)

#define gaPAE_PageTable	((tTabEnt*)PAE_PAGE_TABLE_ADDR)
#define gaPAE_PageDir	((tTabEnt*)PAE_PAGE_DIR_ADDR)
#define gaPAE_MainPDPT	((tTabEnt*)PAE_PAGE_PDPT_ADDR)
#define gaPAE_TmpTable	((tTabEnt*)PAE_TMP_DIR_ADDR)
#define gaPAE_TmpDir	((tTabEnt*)PAE_TMP_DIR_ADDR)
#define gaPAE_TmpPDPT	((tTabEnt*)PAE_TMP_PDPT_ADDR)
 int	gbUsePAE = 0;
tMutex	glTempMappings;
tSemaphore	gTempMappingsSem;
tMutex	glTempFractal;
Uint32	gWorkerStacks[(NUM_WORKER_STACKS+31)/32];
 int	giLastUsedWorker = 0;
struct sPageInfo {
	void	*Node;
	tVAddr	Base;
	Uint64	Offset;
	 int	Length;
	 int	Flags;
}	*gaMappedRegions;	// sizeof = 24 bytes
// - Zero page
tShortSpinlock	glMM_ZeroPage;
tPAddr	giMM_ZeroPage;

// === CODE ===
/**
 * \fn void MM_PreinitVirtual(void)
 * \brief Maps the fractal mappings
 */
void MM_PreinitVirtual(void)
{
	gaInitPageDir[ PAGE_TABLE_ADDR >> 22 ] = ((tTabEnt)&gaInitPageDir - KERNEL_BASE) | 3;
	INVLPG( PAGE_TABLE_ADDR );
	
	Semaphore_Init(&gTempMappingsSem, NUM_TEMP_PAGES, NUM_TEMP_PAGES, "MMVirt", "Temp Mappings");
}

/**
 * \fn void MM_InstallVirtual(void)
 * \brief Sets up the constant page mappings
 */
void MM_InstallVirtual(void)
{
	// Don't bother referencing, as it'a in the kernel area
	//MM_RefPhys( gaInitPageDir[ PAGE_TABLE_ADDR >> 22 ] );
	// --- Pre-Allocate kernel tables
	for( int i = KERNEL_BASE>>22; i < 1024; i ++ )
	{
		if( gaPageDir[ i ] ) {
		//	MM_RefPhys( gaPageDir[ i ] & ~0xFFF );
			continue;
		}	
		// Skip stack tables, they are process unique
		if( i > MM_KERNEL_STACKS >> 22 && i < MM_KERNEL_STACKS_END >> 22) {
			gaPageDir[ i ] = 0;
			continue;
		}
		// Preallocate table
		gaPageDir[ i ] = MM_AllocPhys() | 3;
		INVLPG( &gaPageTable[i*1024] );
		memset( &gaPageTable[i*1024], 0, 0x1000 );
	}
	
	// Unset kernel on the User Text pages
	ASSERT( ((tVAddr)&_UsertextBase & (PAGE_SIZE-1)) == 0 );
	//ASSERT( ((tVAddr)&_UsertextEnd & (PAGE_SIZE-1)) == 0 );
	for( tPage *page = &_UsertextBase; page < &_UsertextEnd; page ++ )
	{
		MM_SetFlags( page, 0, MM_PFLAG_KERNEL );
	}

	// Unmap the area between end of kernel image and the heap
	// DISABLED: Assumptions in main.c
	#if 0
	for( tPage *page = &gKernelEnd; page < (tPage*)(KERNEL_BASE+4*1024*1024); page ++ )
	{
		gaPageTable[ (tVAddr)page / PAGE_SIZE ] = 0;
		//MM_Deallocate(page);
	}
	#endif

	*gpTmpCR3 = 0;
}

/**
 * \brief Cleans up the SMP required mappings
 */
void MM_FinishVirtualInit(void)
{
	gaInitPageDir[ 0 ] = 0;
}

/**
 * \fn void MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs)
 * \brief Called on a page fault
 */
void MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs)
{
	//ENTER("xAddr bErrorCode", Addr, ErrorCode);
	
	// -- Check for COW --
	if( gaPageDir  [Addr>>22] & PF_PRESENT  && gaPageTable[Addr>>12] & PF_PRESENT
	 && gaPageTable[Addr>>12] & PF_COW )
	{
		tPAddr	paddr;
		__asm__ __volatile__ ("sti");
		if(MM_GetRefCount( gaPageTable[Addr>>12] & ~0xFFF ) == 1)
		{
			gaPageTable[Addr>>12] &= ~PF_COW;
			gaPageTable[Addr>>12] |= PF_PRESENT|PF_WRITE;
		}
		else
		{
			//Log("MM_PageFault: COW - MM_DuplicatePage(0x%x)", Addr);
			paddr = MM_DuplicatePage( Addr );
			MM_DerefPhys( gaPageTable[Addr>>12] & ~0xFFF );
			gaPageTable[Addr>>12] &= PF_USER;
			gaPageTable[Addr>>12] |= paddr|PF_PRESENT|PF_WRITE;
		}
		
//		Log_Debug("MMVirt", "COW for %p (%P)", Addr, gaPageTable[Addr>>12]);
		
		INVLPG( Addr & ~0xFFF );
		return;
	}

	// Disable instruction tracing	
	__ASM__("pushf; andw $0xFEFF, 0(%esp); popf");
	Proc_GetCurThread()->bInstrTrace = 0;

	// If it was a user, tell the thread handler
	if(ErrorCode & 4) {
		__asm__ __volatile__ ("sti");
		Log_Warning("MMVirt", "User %s %s memory%s",
			(ErrorCode&2?"write to":"read from"),
			(ErrorCode&1?"bad/locked":"non-present"),
			(ErrorCode&16?" (Instruction Fetch)":"")
			);
		Log_Warning("MMVirt", "Instruction %04x:%08x accessed %p", Regs->cs, Regs->eip, Addr);
		__ASM__("sti");	// Restart IRQs
		#if 1
		Error_Backtrace(Regs->eip, Regs->ebp);
		#endif
		Threads_SegFault(Addr);
		return ;
	}
	
	Debug_KernelPanic();
	
	// -- Check Error Code --
	if(ErrorCode & 8)
		Warning("Reserved Bits Trashed!");
	else
	{
		Warning("Kernel %s %s memory%s",
			(ErrorCode&2?"write to":"read from"),
			(ErrorCode&1?"bad/locked":"non-present"),
			(ErrorCode&16?" (Instruction Fetch)":"")
			);
	}
	
	Log("CPU %i - Code at %p accessed %p", GetCPUNum(), Regs->eip, Addr);
	// Print Stack Backtrace
	Error_Backtrace(Regs->eip, Regs->ebp);

	#if 0	
	Log("gaPageDir[0x%x] = 0x%x", Addr>>22, gaPageDir[Addr>>22]);
	if( gaPageDir[Addr>>22] & PF_PRESENT )
		Log("gaPageTable[0x%x] = 0x%x", Addr>>12, gaPageTable[Addr>>12]);
	#endif
	//MM_DumpTables(0, -1);	
	
	// Register Dump
	Log("EAX %08x ECX %08x EDX %08x EBX %08x", Regs->eax, Regs->ecx, Regs->edx, Regs->ebx);
	Log("ESP %08x EBP %08x ESI %08x EDI %08x", Regs->esp, Regs->ebp, Regs->esi, Regs->edi);
	//Log("SS:ESP %04x:%08x", Regs->ss, Regs->esp);
	Log("CS:EIP %04x:%08x", Regs->cs, Regs->eip);
	Log("DS %04x ES %04x FS %04x GS %04x", Regs->ds, Regs->es, Regs->fs, Regs->gs);
	{
		Uint	dr0, dr1;
		__ASM__ ("mov %%dr0, %0":"=r"(dr0):);
		__ASM__ ("mov %%dr1, %0":"=r"(dr1):);
		Log("DR0 %08x DR1 %08x", dr0, dr1);
	}
	
	Panic("Page Fault at 0x%x (Accessed 0x%x)", Regs->eip, Addr);
}

void MM_DumpTables_Print(tVAddr Start, Uint32 Orig, size_t Size, void *Node)
{
	if( (Orig & ~(PAGE_SIZE-1)) == giMM_ZeroPage )
	{
		Log( "0x%08x => ZERO + 0x%08x (%s%s%s%s%s) %p",
			Start,
			Size,
			(Orig & PF_NOPAGE ? "P" : "-"),
			(Orig & PF_COW ? "C" : "-"),
			(Orig & PF_GLOBAL ? "G" : "-"),
			(Orig & PF_USER ? "U" : "-"),
			(Orig & PF_WRITE ? "W" : "-"),
			Node
			);
	}
	else
	{
		Log(" 0x%08x => 0x%08x + 0x%08x (%s%s%s%s%s) %p",
			Start,
			Orig & ~0xFFF,
			Size,
			(Orig & PF_NOPAGE ? "P" : "-"),
			(Orig & PF_COW ? "C" : "-"),
			(Orig & PF_GLOBAL ? "G" : "-"),
			(Orig & PF_USER ? "U" : "-"),
			(Orig & PF_WRITE ? "W" : "-"),
			Node
			);
	}
}

/**
 * \fn void MM_DumpTables(tVAddr Start, tVAddr End)
 * \brief Dumps the layout of the page tables
 */
void MM_DumpTables(tVAddr Start, tVAddr End)
{
	tVAddr	rangeStart = 0;
	tPAddr	expected = 0;
	void	*expected_node = NULL, *tmpnode = NULL;
	tVAddr	curPos;
	Uint	page;
	const tPAddr	MASK = ~0xF78;
	
	Start >>= 12;	End >>= 12;
	
	#if 0
	Log("Directory Entries:");
	for(page = Start >> 10;
		page < (End >> 10)+1;
		page ++)
	{
		if(gaPageDir[page])
		{
			Log(" 0x%08x-0x%08x :: 0x%08x",
				page<<22, ((page+1)<<22)-1,
				gaPageDir[page]&~0xFFF
				);
		}
	}
	#endif
	
	Log("Table Entries:");
	for(page = Start, curPos = Start<<12;
		page < End;
		curPos += 0x1000, page++)
	{
		if( !(gaPageDir[curPos>>22] & PF_PRESENT)
		||  !(gaPageTable[page] & PF_PRESENT)
		||  (gaPageTable[page] & MASK) != expected
		||  (tmpnode=NULL,MM_GetPageNode(expected, &tmpnode), tmpnode != expected_node))
		{
			if(expected) {
				tPAddr	orig = gaPageTable[rangeStart>>12];
				MM_DumpTables_Print(rangeStart, orig, curPos - rangeStart, expected_node);
				expected = 0;
			}
			if( !(gaPageDir[curPos>>22] & PF_PRESENT) )	continue;
			if( !(gaPageTable[curPos>>12] & PF_PRESENT) )	continue;
			
			expected = (gaPageTable[page] & MASK);
			MM_GetPageNode(expected, &expected_node);
			rangeStart = curPos;
		}
		if(expected && (expected & ~(PAGE_SIZE-1)) != giMM_ZeroPage)
			expected += 0x1000;
	}
	
	if(expected) {
		tPAddr	orig = gaPageTable[rangeStart>>12];
		MM_DumpTables_Print(rangeStart, orig, curPos - rangeStart, expected_node);
		expected = 0;
	}
}

/**
 * \fn tPAddr MM_Allocate(tVAddr VAddr)
 */
tPAddr MM_Allocate(volatile void * VAddr)
{
	tPAddr	paddr = MM_AllocPhys();
	if( MM_Map(VAddr, paddr) )
	{
		return paddr;
	}
	
	// Error of some form, either an overwrite or OOM
	MM_DerefPhys(paddr);
	
	// Check for overwrite
	paddr = MM_GetPhysAddr(VAddr);
	if( paddr != 0 ) {
		Warning("MM_Allocate - Allocating to used address (%p)", VAddr);
		return paddr;
	}
	
	// OOM
	Warning("MM_Allocate - Out of Memory (Called by %p)", __builtin_return_address(0));
	return 0;
}

void MM_AllocateZero(volatile void *VAddr)
{
	if( MM_GetPhysAddr(VAddr) ) {
		Warning("MM_AllocateZero - Attempted overwrite at %p", VAddr);
		return ;
	}
	if( !giMM_ZeroPage )
	{
		SHORTLOCK(&glMM_ZeroPage);
		// Check again within the lock (just in case we lost the race)
		if( giMM_ZeroPage == 0 )
		{
			giMM_ZeroPage = MM_Allocate(VAddr);
			// - Reference a second time to prevent it from being freed
			MM_RefPhys(giMM_ZeroPage);
			memset((void*)VAddr, 0, PAGE_SIZE);
		}
		SHORTREL(&glMM_ZeroPage);
	}
	else
	{
		MM_Map(VAddr, giMM_ZeroPage);
		MM_RefPhys(giMM_ZeroPage);
	}
	MM_SetFlags(VAddr, MM_PFLAG_COW, MM_PFLAG_COW);
}

/**
 * \fn int MM_Map(tVAddr VAddr, tPAddr PAddr)
 * \brief Map a physical page to a virtual one
 */
int MM_Map(volatile void *VAddr, tPAddr PAddr)
{
	Uint	pagenum = (tVAddr)VAddr >> 12;
	
	#if TRACE_MAPS
	Debug("MM_Map(%p, %P)", VAddr, PAddr);
	#endif

	// Sanity check
	if( (PAddr & 0xFFF) || ((tVAddr)VAddr & 0xFFF) ) {
		Log_Warning("MM_Virt", "MM_Map - Physical or Virtual Addresses are not aligned (%P and %p) - %p",
			PAddr, VAddr, __builtin_return_address(0));
		//LEAVE('i', 0);
		return 0;
	}
	
	bool	is_user = ((tVAddr)VAddr < MM_USER_MAX);

	// Check if the directory is mapped
	if( gaPageDir[ pagenum >> 10 ] == 0 )
	{
		tPAddr	tmp = MM_AllocPhys();
		if( tmp == 0 )
			return 0;
		gaPageDir[ pagenum >> 10 ] = tmp | 3 | (is_user ? PF_USER : 0);
		
		INVLPG( &gaPageTable[ pagenum & ~0x3FF ] );
		memsetd( &gaPageTable[ pagenum & ~0x3FF ], 0, 1024 );
	}
	// Check if the page is already allocated
	else if( gaPageTable[ pagenum ] != 0 ) {
		Warning("MM_Map - Allocating to used address");
		//LEAVE('i', 0);
		return 0;
	}
	
	// Map
	gaPageTable[ pagenum ] = PAddr | 3 | (is_user ? PF_USER : 0);
	
	INVLPG( VAddr );
	
	return 1;
}

/*
 * A.k.a MM_Unmap
 */
void MM_Deallocate(volatile void *VAddr)
{
	Uint	pagenum = (tVAddr)VAddr >> 12;
	if( gaPageDir[pagenum>>10] == 0 ) {
		Warning("MM_Deallocate - Directory not mapped");
		return;
	}
	
	if(gaPageTable[pagenum] == 0) {
		Warning("MM_Deallocate - Page is not allocated");
		return;
	}
	
	// Dereference and clear page
	tPAddr	paddr = gaPageTable[pagenum] & ~0xFFF;
	gaPageTable[pagenum] = 0;
	MM_DerefPhys( paddr );
}

/**
 * \fn tPAddr MM_GetPhysAddr(tVAddr Addr)
 * \brief Checks if the passed address is accesable
 */
tPAddr MM_GetPhysAddr(volatile const void *Addr)
{
	tVAddr	addr = (tVAddr)Addr;
	if( !(gaPageDir[addr >> 22] & 1) )
		return 0;
	if( !(gaPageTable[addr >> 12] & 1) )
		return 0;
	return (gaPageTable[addr >> 12] & ~0xFFF) | (addr & 0xFFF);
}

/**
 * \fn void MM_SetCR3(Uint CR3)
 * \brief Sets the current process space
 */
void MM_SetCR3(Uint CR3)
{
	__ASM__("mov %0, %%cr3"::"r"(CR3));
}

/**
 * \brief Clear user's address space
 */
void MM_ClearUser(void)
{
	Uint	i, j;
	
	for( i = 0; i < (MM_USER_MAX>>22); i ++ )
	{
		// Check if directory is not allocated
		if( !(gaPageDir[i] & PF_PRESENT) ) {
			gaPageDir[i] = 0;
			continue;
		}
		
		// Deallocate tables
		for( j = 0; j < 1024; j ++ )
		{
			if( gaPageTable[i*1024+j] & 1 )
				MM_DerefPhys( gaPageTable[i*1024+j] & ~0xFFF );
			gaPageTable[i*1024+j] = 0;
		}
		
		// Deallocate directory
		MM_DerefPhys( gaPageDir[i] & ~0xFFF );
		gaPageDir[i] = 0;
		INVLPG( &gaPageTable[i*1024] );
	}
	INVLPG( gaPageDir );
}

/**
 * \brief Deallocate an address space
 */
void MM_ClearSpace(Uint32 CR3)
{
	 int	i, j;
	
	if(CR3 == (*gpPageCR3 & ~0xFFF)) {
		Log_Error("MMVirt", "Can't clear current address space");
		return ;
	}

	if( MM_GetRefCount(CR3) > 1 ) {
		MM_DerefPhys(CR3);
		Log_Log("MMVirt", "CR3 %P is still referenced, not cleaning (but dereferenced)", CR3);
		return ;
	}

	Log_Debug("MMVirt", "Clearing out address space 0x%x from 0x%x", CR3, *gpPageCR3);
	
	GET_TEMP_MAPPING(CR3);
	INVLPG( gaTmpDir );

	for( i = 0; i < 1024; i ++ )
	{
		Uint32	*table = &gaTmpTable[i*1024];
		if( !(gaTmpDir[i] & PF_PRESENT) )
			continue ;

		INVLPG( table );	

		if( i < 768 || (i > MM_KERNEL_STACKS >> 22 && i < MM_KERNEL_STACKS_END >> 22) )
		{
			for( j = 0; j < 1024; j ++ )
			{
				if( !(table[j] & 1) )
					continue;
				MM_DerefPhys( table[j] & ~0xFFF );
			}
		}

		if( i != (PAGE_TABLE_ADDR >> 22) )
		{		
			MM_DerefPhys( gaTmpDir[i] & ~0xFFF );
		}
	}


	MM_DerefPhys( CR3 );

	REL_TEMP_MAPPING();
}

/**
 * \fn tPAddr MM_Clone(void)
 * \brief Clone the current address space
 */
tPAddr MM_Clone(int bNoUserCopy)
{
	Uint	i, j;
	tPAddr	ret;
	Uint	page = 0;
	tVAddr	kStackBase = Proc_GetCurThread()->KernelStack - MM_KERNEL_STACK_SIZE;
	
	// Create Directory Table
	ret = MM_AllocPhys();
	if( ret == 0 ) {
		return 0;
	}
	
	// Map
	GET_TEMP_MAPPING( ret );
	INVLPG( gaTmpDir );
	memsetd( gaTmpDir, 0, 1024 );
	
	if( Threads_GetPID() != 0 && !bNoUserCopy )
	{	
		// Copy Tables
		for( i = 0; i < 768; i ++)
		{
			// Check if table is allocated
			if( !(gaPageDir[i] & PF_PRESENT) ) {
				gaTmpDir[i] = 0;
				page += 1024;
				continue;
			}
			
			// Allocate new table
			gaTmpDir[i] = MM_AllocPhys() | (gaPageDir[i] & 7);
			INVLPG( &gaTmpTable[page] );
			// Fill
			for( j = 0; j < 1024; j ++, page++ )
			{
				if( !(gaPageTable[page] & PF_PRESENT) ) {
					gaTmpTable[page] = 0;
					continue;
				}
				
				// Refrence old page
				MM_RefPhys( gaPageTable[page] & ~0xFFF );
				// Add to new table
				if(gaPageTable[page] & PF_WRITE) {
					gaTmpTable[page] = (gaPageTable[page] & ~PF_WRITE) | PF_COW;
					gaPageTable[page] = (gaPageTable[page] & ~PF_WRITE) | PF_COW;
					INVLPG( page << 12 );
				}
				else
					gaTmpTable[page] = gaPageTable[page];
			}
		}
	}
	
	// Map in kernel tables (and make fractal mapping)
	for( i = 768; i < 1024; i ++ )
	{
		// Fractal
		if( i == (PAGE_TABLE_ADDR >> 22) ) {
			gaTmpDir[ PAGE_TABLE_ADDR >> 22 ] = *gpTmpCR3;
			continue;
		}
		if( i == (TMP_TABLE_ADDR >> 22) ) {
			gaTmpDir[ TMP_TABLE_ADDR >> 22 ] = 0;
			continue ;
		}
		
		if( gaPageDir[i] == 0 ) {
			gaTmpDir[i] = 0;
			continue;
		}
		
		//LOG("gaPageDir[%x/4] = 0x%x", i*4, gaPageDir[i]);
		MM_RefPhys( gaPageDir[i] & ~0xFFF );
		gaTmpDir[i] = gaPageDir[i];
	}
	
	// Allocate kernel stack
	for(i = MM_KERNEL_STACKS >> 22; i < MM_KERNEL_STACKS_END >> 22; i ++ )
	{
		// Check if directory is allocated
		if( (gaPageDir[i] & 1) == 0 ) {
			gaTmpDir[i] = 0;
			continue;
		}		
		
		// We don't care about other kernel stacks, just the current one
		if( i != kStackBase >> 22 ) {
			MM_DerefPhys( gaPageDir[i] & ~0xFFF );
			gaTmpDir[i] = 0;
			continue;
		}
		
		// Create a copy
		gaTmpDir[i] = MM_AllocPhys() | 3;
		INVLPG( &gaTmpTable[i*1024] );
		for( j = 0; j < 1024; j ++ )
		{
			// Is the page allocated? If not, skip
			if( !(gaPageTable[i*1024+j] & 1) ) {
				gaTmpTable[i*1024+j] = 0;
				continue;
			}
			
			// We don't care about other kernel stacks
			if( ((i*1024+j)*4096 & ~(MM_KERNEL_STACK_SIZE-1)) != kStackBase ) {
				gaTmpTable[i*1024+j] = 0;
				continue;
			}
			
			// Allocate page
			gaTmpTable[i*1024+j] = MM_AllocPhys() | 3;
			
			void *tmp = MM_MapTemp( gaTmpTable[i*1024+j] & ~0xFFF );
			memcpy( tmp, (void *)( (i*1024+j)*PAGE_SIZE ), PAGE_SIZE );
			MM_FreeTemp( tmp );
		}
	}
	
	REL_TEMP_MAPPING();
	
	//LEAVE('x', ret);
	return ret;
}

/**
 * \fn tVAddr MM_NewKStack(void)
 * \brief Create a new kernel stack
 */
tVAddr MM_NewKStack(void)
{
	for(tVAddr base = MM_KERNEL_STACKS; base < MM_KERNEL_STACKS_END; base += MM_KERNEL_STACK_SIZE)
	{
		tPage	*pageptr = (void*)base;
		// Check if space is free
		if(MM_GetPhysAddr(pageptr) != 0)
			continue;
		// Allocate
		for(Uint i = 0; i < MM_KERNEL_STACK_SIZE/PAGE_SIZE; i ++ )
		{
			if( MM_Allocate(pageptr + i) == 0 )
			{
				// On error, print a warning and return error
				Warning("MM_NewKStack - Out of memory");
				// - Clean up
				//for( i += 0x1000 ; i < MM_KERNEL_STACK_SIZE; i += 0x1000 )
				//	MM_Deallocate(base+i);
				return 0;
			}
		}
		// Success
//		Log("MM_NewKStack - Allocated %p", base + MM_KERNEL_STACK_SIZE);
		return base+MM_KERNEL_STACK_SIZE;
	}
	// No stacks left
	Log_Warning("MMVirt", "MM_NewKStack - No address space left");
	return 0;
}

/**
 * \fn tVAddr MM_NewWorkerStack()
 * \brief Creates a new worker stack
 */
tVAddr MM_NewWorkerStack(Uint *StackContents, size_t ContentsSize)
{
	Uint	base;
	tPAddr	page;
	
	LOG("(StackContents=%p,ContentsSize=%i)", StackContents, ContentsSize);
	// TODO: Thread safety
	// Find a free worker stack address
	for(base = giLastUsedWorker; base < NUM_WORKER_STACKS; base++)
	{
		// Used block
		if( gWorkerStacks[base/32] == -1 ) {
			base += 31;	base &= ~31;
			base --;	// Counteracted by the base++
			continue;
		}
		// Used stack
		if( gWorkerStacks[base/32] & (1 << base) ) {
			continue;
		}
		break;
	}
	if(base >= NUM_WORKER_STACKS) {
		Log_Error("MMVirt", "Uh-oh! Out of worker stacks");
		return 0;
	}
	LOG("base=0x%x", base);
	
	// It's ours now!
	gWorkerStacks[base/32] |= (1 << base);
	// Make life easier for later calls
	giLastUsedWorker = base;
	// We have one
	base = WORKER_STACKS + base * WORKER_STACK_SIZE;
	//Log(" MM_NewWorkerStack: base = 0x%x", base);
	LOG("base=%p (top)", base);
	
	// Set the temp fractals to TID0's address space
	GET_TEMP_MAPPING( ((Uint)gaInitPageDir - KERNEL_BASE) );
	INVLPG( gaTmpDir );
	
	// Check if the directory is mapped (we are assuming that the stacks
	// will fit neatly in a directory)
	LOG("gaTmpDir[ 0x%x ] = 0x%x", base>>22, gaTmpDir[ base >> 22 ]);
	if(gaTmpDir[ base >> 22 ] == 0) {
		gaTmpDir[ base >> 22 ] = MM_AllocPhys() | 3;
		INVLPG( &gaTmpTable[ (base>>12) & ~0x3FF ] );
	}
	
	// Mapping Time!
	for( Uint addr = 0; addr < WORKER_STACK_SIZE; addr += 0x1000 )
	{
		page = MM_AllocPhys();
		gaTmpTable[ (base + addr) >> 12 ] = page | 3;
	}
	LOG("mapped");

	// Release temporary fractal
	REL_TEMP_MAPPING();

	// NOTE: Max of 1 page
	// `page` is the last allocated page from the previious for loop
	LOG("Mapping first page");
	char	*tmpPage = MM_MapTemp( page );
	LOG("tmpPage=%p", tmpPage);
	memcpy( tmpPage + (0x1000 - ContentsSize), StackContents, ContentsSize);
	MM_FreeTemp( tmpPage );
	
	//Log("MM_NewWorkerStack: RETURN 0x%x", base);
	LOG("return %p", base+WORKER_STACK_SIZE);
	return base + WORKER_STACK_SIZE;
}

/**
 * \fn void MM_SetFlags(tVAddr VAddr, Uint Flags, Uint Mask)
 * \brief Sets the flags on a page
 */
void MM_SetFlags(volatile void *VAddr, Uint Flags, Uint Mask)
{
	Uint	pagenum = (tVAddr)VAddr >> 12;
	if( !(gaPageDir[pagenum >> 10] & 1) )	return ;
	if( !(gaPageTable[pagenum] & 1) )	return ;
	
	tTabEnt	*ent = &gaPageTable[pagenum];
	
	// Read-Only
	if( Mask & MM_PFLAG_RO )
	{
		if( Flags & MM_PFLAG_RO ) {
			*ent &= ~PF_WRITE;
		}
		else {
			gaPageDir[pagenum >> 10] |= PF_WRITE;
			*ent |= PF_WRITE;
		}
	}
	
	// Kernel
	if( Mask & MM_PFLAG_KERNEL )
	{
		if( Flags & MM_PFLAG_KERNEL ) {
			*ent &= ~PF_USER;
		}
		else {
			gaPageDir[pagenum >> 10] |= PF_USER;
			*ent |= PF_USER;
		}
	}
	
	// Copy-On-Write
	if( Mask & MM_PFLAG_COW )
	{
		if( Flags & MM_PFLAG_COW ) {
			*ent &= ~PF_WRITE;
			*ent |= PF_COW;
		}
		else {
			*ent &= ~PF_COW;
			*ent |= PF_WRITE;
		}
	}
	
	//Log("MM_SetFlags: *ent = 0x%08x, gaPageDir[%i] = 0x%08x",
	//	*ent, VAddr >> 22, gaPageDir[VAddr >> 22]);
}

/**
 * \brief Get the flags on a page
 */
Uint MM_GetFlags(volatile const void *VAddr)
{
	Uint	pagenum = (tVAddr)VAddr >> 12;
	
	// Validity Check
	if( !(gaPageDir[pagenum >> 10] & 1) )	return 0;
	if( !(gaPageTable[pagenum] & 1) )	return 0;
	
	tTabEnt	*ent = &gaPageTable[pagenum];
	
	Uint	ret = 0;
	// Read-Only
	if( !(*ent & PF_WRITE) )	ret |= MM_PFLAG_RO;
	// Kernel
	if( !(*ent & PF_USER) )	ret |= MM_PFLAG_KERNEL;
	// Copy-On-Write
	if( *ent & PF_COW )	ret |= MM_PFLAG_COW;
	
	return ret;
}

/**
 * \brief Check if the provided buffer is valid
 * \return Boolean valid
 */
int MM_IsValidBuffer(tVAddr Addr, size_t Size)
{
	 int	bIsUser;
	 int	dir, tab;

	Size += Addr & (PAGE_SIZE-1);
	Addr &= ~(PAGE_SIZE-1);

	dir = Addr >> 22;
	tab = Addr >> 12;
	
//	Debug("Addr = %p, Size = 0x%x, dir = %i, tab = %i", Addr, Size, dir, tab);

	if( !(gaPageDir[dir] & 1) )	return 0;
	if( !(gaPageTable[tab] & 1) )	return 0;
	
	bIsUser = !!(gaPageTable[tab] & PF_USER);

	while( Size >= PAGE_SIZE )
	{
		if( (tab & 1023) == 0 )
		{
			dir ++;
			if( !(gaPageDir[dir] & 1) )	return 0;
		}
		
		if( !(gaPageTable[tab] & 1) )   return 0;
		if( bIsUser && !(gaPageTable[tab] & PF_USER) )	return 0;

		tab ++;
		Size -= PAGE_SIZE;
	}
	return 1;
}

/**
 * \fn tPAddr MM_DuplicatePage(tVAddr VAddr)
 * \brief Duplicates a virtual page to a physical one
 */
tPAddr MM_DuplicatePage(tVAddr VAddr)
{
	tPAddr	ret;
	void	*temp;
	 int	wasRO = 0;
	
	//ENTER("xVAddr", VAddr);
	
	// Check if mapped
	if( !(gaPageDir  [VAddr >> 22] & PF_PRESENT) )	return 0;
	if( !(gaPageTable[VAddr >> 12] & PF_PRESENT) )	return 0;
	
	// Page Align
	VAddr &= ~0xFFF;
	
	// Allocate new page
	ret = MM_AllocPhys();
	if( !ret ) {
		return 0;
	}
	
	// Write-lock the page (to keep data constistent), saving its R/W state
	wasRO = (gaPageTable[VAddr >> 12] & PF_WRITE ? 0 : 1);
	gaPageTable[VAddr >> 12] &= ~PF_WRITE;
	INVLPG( VAddr );
	
	// Copy Data
	temp = MM_MapTemp(ret);
	memcpy( temp, (void*)VAddr, 0x1000 );
	MM_FreeTemp(temp);
	
	// Restore Writeable status
	if(!wasRO)	gaPageTable[VAddr >> 12] |= PF_WRITE;
	INVLPG(VAddr);
	
	//LEAVE('X', ret);
	return ret;
}

/**
 * \fn Uint MM_MapTemp(tPAddr PAddr)
 * \brief Create a temporary memory mapping
 * \todo Show Luigi Barone (C Lecturer) and see what he thinks
 */
void *MM_MapTemp(tPAddr PAddr)
{
	ENTER("PPAddr", PAddr);
	
	PAddr &= ~0xFFF;
	
	if( Semaphore_Wait(&gTempMappingsSem, 1) != 1 )
		return NULL;
	LOG("Semaphore good");
	Mutex_Acquire( &glTempMappings );
	for( int i = 0; i < NUM_TEMP_PAGES; i ++ )
	{
		Uint32	*pte = &gaPageTable[ (TEMP_MAP_ADDR >> 12) + i ];
		LOG("%i: %x", i, *pte);
		// Check if page used
		if(*pte & 1)	continue;
		// Mark as used
		*pte = PAddr | 3;
		INVLPG( TEMP_MAP_ADDR + (i << 12) );
		LEAVE('p', TEMP_MAP_ADDR + (i << 12));
		Mutex_Release( &glTempMappings );
		return (void*)( TEMP_MAP_ADDR + (i << 12) );
	}
	Mutex_Release( &glTempMappings );
	Log_KernelPanic("MMVirt", "Semaphore suplied a mapping, but none are avaliable");
	return NULL;
}

/**
 * \fn void MM_FreeTemp(tVAddr PAddr)
 * \brief Free's a temp mapping
 */
void MM_FreeTemp(void *VAddr)
{
	 int	i = (tVAddr)VAddr >> 12;
	//ENTER("xVAddr", VAddr);
	
	if(i >= (TEMP_MAP_ADDR >> 12)) {
		gaPageTable[ i ] = 0;
		Semaphore_Signal(&gTempMappingsSem, 1);
	}
	
	//LEAVE('-');
}

/**
 * \fn tVAddr MM_MapHWPages(tPAddr PAddr, Uint Number)
 * \brief Allocates a contigous number of pages
 */
void *MM_MapHWPages(tPAddr PAddr, Uint Number)
{
	 int	j;
	
	PAddr &= ~0xFFF;

	if( PAddr < 1024*1024 && (1024*1024-PAddr) >= Number * PAGE_SIZE )
	{
		return (void*)(KERNEL_BASE + PAddr);
	}

	// Scan List
	for( int i = 0; i < NUM_HW_PAGES; i ++ )
	{		
		// Check if addr used
		if( gaPageTable[ (HW_MAP_ADDR >> 12) + i ] & 1 )
			continue;
		
		// Check possible region
		for( j = 0; j < Number && i + j < NUM_HW_PAGES; j ++ )
		{
			// If there is an allocated page in the region we are testing, break
			if( gaPageTable[ (HW_MAP_ADDR >> 12) + i + j ] & 1 )	break;
		}
		// Is it all free?
		if( j == Number )
		{
			// Allocate
			for( j = 0; j < Number; j++ ) {
				MM_RefPhys( PAddr + (j<<12) );
				gaPageTable[ (HW_MAP_ADDR >> 12) + i + j ] = (PAddr + (j<<12)) | 3;
			}
			return (void*)(HW_MAP_ADDR + (i<<12));
		}
	}
	// If we don't find any, return NULL
	return 0;
}

/**
 * \fn tVAddr MM_AllocDMA(int Pages, int MaxBits, tPAddr *PhysAddr)
 * \brief Allocates DMA physical memory
 * \param Pages	Number of pages required
 * \param MaxBits	Maximum number of bits the physical address can have
 * \param PhysAddr	Pointer to the location to place the physical address allocated
 * \return Virtual address allocate
 */
void *MM_AllocDMA(int Pages, int MaxBits, tPAddr *PhysAddr)
{
	tPAddr	phys;
	void	*ret;
	
	ENTER("iPages iMaxBits pPhysAddr", Pages, MaxBits, PhysAddr);
	
	if(MaxBits == -1)
		MaxBits = PHYS_BITS;
	
	// Sanity Check
	if(MaxBits < 12) {
		LEAVE('i', 0);
		return 0;
	}
	
	// Fast Allocate
	if(Pages == 1 && MaxBits >= PHYS_BITS)
	{
		phys = MM_AllocPhys();
		if( PhysAddr )
			*PhysAddr = phys;
		if( !phys ) {
			LEAVE_RET('i', 0);
		}
		ret = MM_MapHWPages(phys, 1);
		if(ret == 0) {
			MM_DerefPhys(phys);
			LEAVE('i', 0);
			return 0;
		}
		LEAVE('x', ret);
		return (void*)ret;
	}
	
	// Slow Allocate
	phys = MM_AllocPhysRange(Pages, MaxBits);
	// - Was it allocated?
	if(phys == 0) {
		LEAVE('i', 0);
		return 0;
	}
	
	// Allocated successfully, now map
	ret = MM_MapHWPages(phys, Pages);
	// - MapHWPages references the memory, so release references
	for( int i = 0; i < Pages; i ++ )
		MM_DerefPhys(phys + i*PAGE_SIZE);
	if( ret == 0 ) {
		LEAVE('i', 0);
		return 0;
	}
	
	if( PhysAddr )
		*PhysAddr = phys;
	LEAVE('x', ret);
	return (void*)ret;
}

/**
 * \fn void MM_UnmapHWPages(tVAddr VAddr, Uint Number)
 * \brief Unmap a hardware page
 */
void MM_UnmapHWPages(volatile void *Base, Uint Number)
{
	tVAddr	VAddr = (tVAddr)Base;
	//Log_Debug("VirtMem", "MM_UnmapHWPages: (VAddr=0x%08x, Number=%i)", VAddr, Number);

	//
	if( KERNEL_BASE <= VAddr && VAddr < KERNEL_BASE + 1024*1024 )
		return ;
	
	Uint pagenum = VAddr >> 12;

	// Sanity Check
	if(VAddr < HW_MAP_ADDR || VAddr+Number*0x1000 > HW_MAP_MAX)	return;
	
	
	Mutex_Acquire( &glTempMappings );	// Temp and HW share a directory, so they share a lock
	
	for( Uint i = 0; i < Number; i ++ )
	{
		MM_DerefPhys( gaPageTable[ pagenum + i ] & ~0xFFF );
		gaPageTable[ pagenum + i ] = 0;
		INVLPG( (tVAddr)(pagenum + i) << 12 );
	}
	
	Mutex_Release( &glTempMappings );
}

