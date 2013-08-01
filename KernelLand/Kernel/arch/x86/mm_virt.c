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

#define TAB	22

#define WORKER_STACKS		0x00100000	// Thread0 Only!
#define	WORKER_STACK_SIZE	MM_KERNEL_STACK_SIZE
#define WORKER_STACKS_END	0xB0000000
#define	NUM_WORKER_STACKS	((WORKER_STACKS_END-WORKER_STACKS)/WORKER_STACK_SIZE)

#define PAE_PAGE_TABLE_ADDR	0xFC000000	// 16 MiB
#define PAE_PAGE_DIR_ADDR	0xFCFC0000	// 16 KiB
#define PAE_PAGE_PDPT_ADDR	0xFCFC3F00	// 32 bytes
#define PAE_TMP_PDPT_ADDR	0xFCFC3F20	// 32 bytes
#define PAE_TMP_DIR_ADDR	0xFCFE0000	// 16 KiB
#define PAE_TMP_TABLE_ADDR	0xFD000000	// 16 MiB

#define PAGE_TABLE_ADDR	0xFC000000
#define PAGE_DIR_ADDR	0xFC3F0000
#define PAGE_CR3_ADDR	0xFC3F0FC0
#define TMP_CR3_ADDR	0xFC3F0FC4	// Part of core instead of temp
#define TMP_DIR_ADDR	0xFC3F1000	// Same
#define TMP_TABLE_ADDR	0xFC400000

#define HW_MAP_ADDR		0xFE000000
#define	HW_MAP_MAX		0xFFEF0000
#define	NUM_HW_PAGES	((HW_MAP_MAX-HW_MAP_ADDR)/0x1000)
#define	TEMP_MAP_ADDR	0xFFEF0000	// Allows 16 "temp" pages
#define	NUM_TEMP_PAGES	16
#define LAST_BLOCK_ADDR	0xFFFF0000	// Free space for kernel provided user code/ *(-1) protection

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
extern char	_UsertextEnd[], _UsertextBase[];
extern Uint32	gaInitPageDir[1024];
extern Uint32	gaInitPageTable[1024];
extern void	Threads_SegFault(tVAddr Addr);
extern void	Error_Backtrace(Uint eip, Uint ebp);

// === PROTOTYPES ===
void	MM_PreinitVirtual(void);
void	MM_InstallVirtual(void);
void	MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs);
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
	 int	i;
	
	// --- Pre-Allocate kernel tables
	for( i = KERNEL_BASE>>22; i < 1024; i ++ )
	{
		if( gaPageDir[ i ] )	continue;
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
	for( i = ((tVAddr)&_UsertextEnd-(tVAddr)&_UsertextBase+0xFFF)/4096; i--; ) {
		MM_SetFlags( (tVAddr)&_UsertextBase + i*4096, 0, MM_PFLAG_KERNEL );
	}
	
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
				Log(" 0x%08x => 0x%08x - 0x%08x (%s%s%s%s%s) %p",
					rangeStart,
					orig & ~0xFFF,
					curPos - rangeStart,
					(orig & PF_NOPAGE ? "P" : "-"),
					(orig & PF_COW ? "C" : "-"),
					(orig & PF_GLOBAL ? "G" : "-"),
					(orig & PF_USER ? "U" : "-"),
					(orig & PF_WRITE ? "W" : "-"),
					expected_node
					);
				expected = 0;
			}
			if( !(gaPageDir[curPos>>22] & PF_PRESENT) )	continue;
			if( !(gaPageTable[curPos>>12] & PF_PRESENT) )	continue;
			
			expected = (gaPageTable[page] & MASK);
			MM_GetPageNode(expected, &expected_node);
			rangeStart = curPos;
		}
		if(expected)	expected += 0x1000;
	}
	
	if(expected) {
		tPAddr	orig = gaPageTable[rangeStart>>12];
		Log("0x%08x => 0x%08x - 0x%08x (%s%s%s%s%s) %p",
			rangeStart,
			orig & ~0xFFF,
			curPos - rangeStart,
			(orig & PF_NOPAGE ? "p" : "-"),
			(orig & PF_COW ? "C" : "-"),
			(orig & PF_GLOBAL ? "G" : "-"),
			(orig & PF_USER ? "U" : "-"),
			(orig & PF_WRITE ? "W" : "-"),
			expected_node
			);
		expected = 0;
	}
}

/**
 * \fn tPAddr MM_Allocate(tVAddr VAddr)
 */
tPAddr MM_Allocate(tVAddr VAddr)
{
	tPAddr	paddr;
	//ENTER("xVAddr", VAddr);
	//__ASM__("xchg %bx,%bx");
	// Check if the directory is mapped
	if( gaPageDir[ VAddr >> 22 ] == 0 )
	{
		// Allocate directory
		paddr = MM_AllocPhys();
		if( paddr == 0 ) {
			Warning("MM_Allocate - Out of Memory (Called by %p)", __builtin_return_address(0));
			//LEAVE('i',0);
			return 0;
		}
		// Map and mark as user (if needed)
		gaPageDir[ VAddr >> 22 ] = paddr | 3;
		if(VAddr < MM_USER_MAX)	gaPageDir[ VAddr >> 22 ] |= PF_USER;
		
		INVLPG( &gaPageDir[ VAddr >> 22 ] );
		memsetd( &gaPageTable[ (VAddr >> 12) & ~0x3FF ], 0, 1024 );
	}
	// Check if the page is already allocated
	else if( gaPageTable[ VAddr >> 12 ] != 0 ) {
		Warning("MM_Allocate - Allocating to used address (%p)", VAddr);
		//LEAVE('X', gaPageTable[ VAddr >> 12 ] & ~0xFFF);
		return gaPageTable[ VAddr >> 12 ] & ~0xFFF;
	}
	
	// Allocate
	paddr = MM_AllocPhys();
	//LOG("paddr = 0x%llx", paddr);
	if( paddr == 0 ) {
		Warning("MM_Allocate - Out of Memory when allocating at %p (Called by %p)",
			VAddr, __builtin_return_address(0));
		//LEAVE('i',0);
		return 0;
	}
	// Map
	gaPageTable[ VAddr >> 12 ] = paddr | 3;
	// Mark as user
	if(VAddr < MM_USER_MAX)	gaPageTable[ VAddr >> 12 ] |= PF_USER;
	// Invalidate Cache for address
	INVLPG( VAddr & ~0xFFF );
	
	//LEAVE('X', paddr);
	return paddr;
}

/**
 * \fn void MM_Deallocate(tVAddr VAddr)
 */
void MM_Deallocate(tVAddr VAddr)
{
	if( gaPageDir[ VAddr >> 22 ] == 0 ) {
		Warning("MM_Deallocate - Directory not mapped");
		return;
	}
	
	if(gaPageTable[ VAddr >> 12 ] == 0) {
		Warning("MM_Deallocate - Page is not allocated");
		return;
	}
	
	// Dereference page
	MM_DerefPhys( gaPageTable[ VAddr >> 12 ] & ~0xFFF );
	// Clear page
	gaPageTable[ VAddr >> 12 ] = 0;
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
 * \fn int MM_Map(tVAddr VAddr, tPAddr PAddr)
 * \brief Map a physical page to a virtual one
 */
int MM_Map(tVAddr VAddr, tPAddr PAddr)
{
	//ENTER("xVAddr xPAddr", VAddr, PAddr);
	// Sanity check
	if( PAddr & 0xFFF || VAddr & 0xFFF ) {
		Log_Warning("MM_Virt", "MM_Map - Physical or Virtual Addresses are not aligned (0x%P and %p)",
			PAddr, VAddr);
		//LEAVE('i', 0);
		return 0;
	}
	
	// Align addresses
	PAddr &= ~0xFFF;	VAddr &= ~0xFFF;
	
	// Check if the directory is mapped
	if( gaPageDir[ VAddr >> 22 ] == 0 )
	{
		tPAddr	tmp = MM_AllocPhys();
		if( tmp == 0 )
			return 0;
		gaPageDir[ VAddr >> 22 ] = tmp | 3;
		
		// Mark as user
		if(VAddr < MM_USER_MAX)	gaPageDir[ VAddr >> 22 ] |= PF_USER;
		
		INVLPG( &gaPageTable[ (VAddr >> 12) & ~0x3FF ] );
		memsetd( &gaPageTable[ (VAddr >> 12) & ~0x3FF ], 0, 1024 );
	}
	// Check if the page is already allocated
	else if( gaPageTable[ VAddr >> 12 ] != 0 ) {
		Warning("MM_Map - Allocating to used address");
		//LEAVE('i', 0);
		return 0;
	}
	
	// Map
	gaPageTable[ VAddr >> 12 ] = PAddr | 3;
	// Mark as user
	if(VAddr < MM_USER_MAX)	gaPageTable[ VAddr >> 12 ] |= PF_USER;
	
	//LOG("gaPageTable[ 0x%x ] = (Uint)%p = 0x%x",
	//	VAddr >> 12, &gaPageTable[ VAddr >> 12 ], gaPageTable[ VAddr >> 12 ]);
	
	// Reference
	MM_RefPhys( PAddr );
	
	//LOG("INVLPG( 0x%x )", VAddr);
	INVLPG( VAddr );
	
	//LEAVE('i', 1);
	return 1;
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
	void	*tmp;
	
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
			
			MM_RefPhys( gaTmpTable[i*1024+j] & ~0xFFF );
			
			tmp = MM_MapTemp( gaTmpTable[i*1024+j] & ~0xFFF );
			memcpy( tmp, (void *)( (i*1024+j)*0x1000 ), 0x1000 );
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
	tVAddr	base;
	Uint	i;
	for(base = MM_KERNEL_STACKS; base < MM_KERNEL_STACKS_END; base += MM_KERNEL_STACK_SIZE)
	{
		// Check if space is free
		if(MM_GetPhysAddr( (void*) base) != 0)
			continue;
		// Allocate
		//for(i = MM_KERNEL_STACK_SIZE; i -= 0x1000 ; )
		for(i = 0; i < MM_KERNEL_STACK_SIZE; i += 0x1000 )
		{
			if( MM_Allocate(base+i) == 0 )
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
	Uint	base, addr;
	tVAddr	tmpPage;
	tPAddr	page;
	
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
		Warning("Uh-oh! Out of worker stacks");
		return 0;
	}
	
	// It's ours now!
	gWorkerStacks[base/32] |= (1 << base);
	// Make life easier for later calls
	giLastUsedWorker = base;
	// We have one
	base = WORKER_STACKS + base * WORKER_STACK_SIZE;
	//Log(" MM_NewWorkerStack: base = 0x%x", base);
	
	// Set the temp fractals to TID0's address space
	GET_TEMP_MAPPING( ((Uint)gaInitPageDir - KERNEL_BASE) );
	INVLPG( gaTmpDir );
	
	// Check if the directory is mapped (we are assuming that the stacks
	// will fit neatly in a directory)
	//Log(" MM_NewWorkerStack: gaTmpDir[ 0x%x ] = 0x%x", base>>22, gaTmpDir[ base >> 22 ]);
	if(gaTmpDir[ base >> 22 ] == 0) {
		gaTmpDir[ base >> 22 ] = MM_AllocPhys() | 3;
		INVLPG( &gaTmpTable[ (base>>12) & ~0x3FF ] );
	}
	
	// Mapping Time!
	for( addr = 0; addr < WORKER_STACK_SIZE; addr += 0x1000 )
	{
		page = MM_AllocPhys();
		gaTmpTable[ (base + addr) >> 12 ] = page | 3;
	}

	// Release temporary fractal
	REL_TEMP_MAPPING();

	// NOTE: Max of 1 page
	// `page` is the last allocated page from the previious for loop
	tmpPage = (tVAddr)MM_MapTemp( page );
	memcpy( (void*)( tmpPage + (0x1000 - ContentsSize) ), StackContents, ContentsSize);
	MM_FreeTemp( (void*)tmpPage );
	
	//Log("MM_NewWorkerStack: RETURN 0x%x", base);
	return base + WORKER_STACK_SIZE;
}

/**
 * \fn void MM_SetFlags(tVAddr VAddr, Uint Flags, Uint Mask)
 * \brief Sets the flags on a page
 */
void MM_SetFlags(tVAddr VAddr, Uint Flags, Uint Mask)
{
	tTabEnt	*ent;
	if( !(gaPageDir[VAddr >> 22] & 1) )	return ;
	if( !(gaPageTable[VAddr >> 12] & 1) )	return ;
	
	ent = &gaPageTable[VAddr >> 12];
	
	// Read-Only
	if( Mask & MM_PFLAG_RO )
	{
		if( Flags & MM_PFLAG_RO ) {
			*ent &= ~PF_WRITE;
		}
		else {
			gaPageDir[VAddr >> 22] |= PF_WRITE;
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
			gaPageDir[VAddr >> 22] |= PF_USER;
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
Uint MM_GetFlags(tVAddr VAddr)
{
	tTabEnt	*ent;
	Uint	ret = 0;
	
	// Validity Check
	if( !(gaPageDir[VAddr >> 22] & 1) )	return 0;
	if( !(gaPageTable[VAddr >> 12] & 1) )	return 0;
	
	ent = &gaPageTable[VAddr >> 12];
	
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
void * MM_MapTemp(tPAddr PAddr)
{
	//ENTER("XPAddr", PAddr);
	
	PAddr &= ~0xFFF;
	
	//LOG("glTempMappings = %i", glTempMappings);
	
	if( Semaphore_Wait(&gTempMappingsSem, 1) != 1 )
		return NULL;
	Mutex_Acquire( &glTempMappings );
	for( int i = 0; i < NUM_TEMP_PAGES; i ++ )
	{
		// Check if page used
		if(gaPageTable[ (TEMP_MAP_ADDR >> 12) + i ] & 1)	continue;
		// Mark as used
		gaPageTable[ (TEMP_MAP_ADDR >> 12) + i ] = PAddr | 3;
		INVLPG( TEMP_MAP_ADDR + (i << 12) );
		//LEAVE('p', TEMP_MAP_ADDR + (i << 12));
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
	if( ret == 0 ) {
		// If it didn't map, free then return 0
		for(;Pages--;phys+=0x1000)
			MM_DerefPhys(phys);
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
void MM_UnmapHWPages(tVAddr VAddr, Uint Number)
{
	 int	i, j;
	
	//Log_Debug("VirtMem", "MM_UnmapHWPages: (VAddr=0x%08x, Number=%i)", VAddr, Number);

	//
	if( KERNEL_BASE <= VAddr && VAddr < KERNEL_BASE + 1024*1024 )
		return ;	

	// Sanity Check
	if(VAddr < HW_MAP_ADDR || VAddr+Number*0x1000 > HW_MAP_MAX)	return;
	
	i = VAddr >> 12;
	
	Mutex_Acquire( &glTempMappings );	// Temp and HW share a directory, so they share a lock
	
	for( j = 0; j < Number; j++ )
	{
		MM_DerefPhys( gaPageTable[ i + j ] & ~0xFFF );
		gaPageTable[ i + j ] = 0;
		INVLPG( (tVAddr)(i+j) << 12 );
	}
	
	Mutex_Release( &glTempMappings );
}

