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
#include <mm_phys.h>
#include <proc.h>

#if USE_PAE
# define TAB	21
# define DIR	30
#else
# define TAB	22
#endif

#define KERNEL_STACKS		0xF0000000
#define	KERNEL_STACK_SIZE	0x00008000
#define KERNEL_STACKS_END	0xFC000000
#define WORKER_STACKS		0x00100000	// Thread0 Only!
#define	WORKER_STACK_SIZE	KERNEL_STACK_SIZE
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
#define	PF_COW		0x200
#define	PF_PAGED	0x400

#define INVLPG(addr)	__asm__ __volatile__ ("invlpg (%0)"::"r"(addr))

#if USE_PAE
typedef Uint64	tTabEnt;
#else
typedef Uint32	tTabEnt;
#endif

// === IMPORTS ===
extern Uint32	gaInitPageDir[1024];
extern Uint32	gaInitPageTable[1024];
extern void	Threads_SegFault(tVAddr Addr);
extern void	Error_Backtrace(Uint eip, Uint ebp);

// === PROTOTYPES ===
void	MM_PreinitVirtual();
void	MM_InstallVirtual();
void	MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs);
void	MM_DumpTables(tVAddr Start, tVAddr End);
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
 int	gilTempMappings = 0;
 int	gilTempFractal = 0;
Uint32	gWorkerStacks[(NUM_WORKER_STACKS+31)/32];
 int	giLastUsedWorker = 0;

// === CODE ===
/**
 * \fn void MM_PreinitVirtual()
 * \brief Maps the fractal mappings
 */
void MM_PreinitVirtual()
{
	#if USE_PAE
	gaInitPageDir[ ((PAGE_TABLE_ADDR >> TAB)-3*512+3)*2 ] = ((tTabEnt)&gaInitPageDir - KERNEL_BASE) | 3;
	#else
	gaInitPageDir[ PAGE_TABLE_ADDR >> 22 ] = ((tTabEnt)&gaInitPageDir - KERNEL_BASE) | 3;
	#endif
	INVLPG( PAGE_TABLE_ADDR );
}

/**
 * \fn void MM_InstallVirtual()
 * \brief Sets up the constant page mappings
 */
void MM_InstallVirtual()
{
	 int	i;
	
	#if USE_PAE
	// --- Pre-Allocate kernel tables
	for( i = KERNEL_BASE >> TAB; i < 1024*4; i ++ )
	{
		if( gaPAE_PageDir[ i ] )	continue;
		
		// Skip stack tables, they are process unique
		if( i > KERNEL_STACKS >> TAB && i < KERNEL_STACKS_END >> TAB) {
			gaPAE_PageDir[ i ] = 0;
			continue;
		}
		// Preallocate table
		gaPAE_PageDir[ i ] = MM_AllocPhys() | 3;
		INVLPG( &gaPAE_PageTable[i*512] );
		memset( &gaPAE_PageTable[i*512], 0, 0x1000 );
	}
	#else
	// --- Pre-Allocate kernel tables
	for( i = KERNEL_BASE>>22; i < 1024; i ++ )
	{
		if( gaPageDir[ i ] )	continue;
		// Skip stack tables, they are process unique
		if( i > KERNEL_STACKS >> 22 && i < KERNEL_STACKS_END >> 22) {
			gaPageDir[ i ] = 0;
			continue;
		}
		// Preallocate table
		gaPageDir[ i ] = MM_AllocPhys() | 3;
		INVLPG( &gaPageTable[i*1024] );
		memset( &gaPageTable[i*1024], 0, 0x1000 );
	}
	#endif
}

/**
 * \brief Cleans up the SMP required mappings
 */
void MM_FinishVirtualInit()
{
	#if USE_PAE
	gaInitPDPT[ 0 ] = 0;
	#else
	gaInitPageDir[ 0 ] = 0;
	#endif
}

/**
 * \fn void MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs)
 * \brief Called on a page fault
 */
void MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs)
{
	//ENTER("xAddr bErrorCode", Addr, ErrorCode);
	
	// -- Check for COW --
	if( gaPageDir  [Addr>>22] & PF_PRESENT
	 && gaPageTable[Addr>>12] & PF_PRESENT
	 && gaPageTable[Addr>>12] & PF_COW )
	{
		tPAddr	paddr;
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
		
		INVLPG( Addr & ~0xFFF );
		//LEAVE('-')
		return;
	}
	
	// If it was a user, tell the thread handler
	if(ErrorCode & 4) {
		Warning("%s %s %s memory%s",
			(ErrorCode&4?"User":"Kernel"),
			(ErrorCode&2?"write to":"read from"),
			(ErrorCode&1?"bad/locked":"non-present"),
			(ErrorCode&16?" (Instruction Fetch)":"")
			);
		Warning("User Pagefault: Instruction at %04x:%08x accessed %p", Regs->cs, Regs->eip, Addr);
		__asm__ __volatile__ ("sti");	// Restart IRQs
		Threads_SegFault(Addr);
		return ;
	}
	
	// -- Check Error Code --
	if(ErrorCode & 8)
		Warning("Reserved Bits Trashed!");
	else
	{
		Warning("%s %s %s memory%s",
			(ErrorCode&4?"User":"Kernel"),
			(ErrorCode&2?"write to":"read from"),
			(ErrorCode&1?"bad/locked":"non-present"),
			(ErrorCode&16?" (Instruction Fetch)":"")
			);
	}
	
	Log("Code at %p accessed %p", Regs->eip, Addr);
	// Print Stack Backtrace
	Error_Backtrace(Regs->eip, Regs->ebp);
	
	Log("gaPageDir[0x%x] = 0x%x", Addr>>22, gaPageDir[Addr>>22]);
	if( gaPageDir[Addr>>22] & PF_PRESENT )
		Log("gaPageTable[0x%x] = 0x%x", Addr>>12, gaPageTable[Addr>>12]);
	
	//MM_DumpTables(0, -1);	
	
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
	tVAddr	curPos;
	Uint	page;
	const tPAddr	MASK = ~0xF98;
	
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
		||  (gaPageTable[page] & MASK) != expected)
		{
			if(expected) {
				Log(" 0x%08x-0x%08x => 0x%08x-0x%08x (%s%s%s%s)",
					rangeStart, curPos - 1,
					gaPageTable[rangeStart>>12] & ~0xFFF,
					(expected & ~0xFFF) - 1,
					(expected & PF_PAGED ? "p" : "-"),
					(expected & PF_COW ? "C" : "-"),
					(expected & PF_USER ? "U" : "-"),
					(expected & PF_WRITE ? "W" : "-")
					);
				expected = 0;
			}
			if( !(gaPageDir[curPos>>22] & PF_PRESENT) )	continue;
			if( !(gaPageTable[curPos>>12] & PF_PRESENT) )	continue;
			
			expected = (gaPageTable[page] & MASK);
			rangeStart = curPos;
		}
		if(expected)	expected += 0x1000;
	}
	
	if(expected) {
		Log("0x%08x-0x%08x => 0x%08x-0x%08x (%s%s%s%s)",
			rangeStart, curPos - 1,
			gaPageTable[rangeStart>>12] & ~0xFFF,
			(expected & ~0xFFF) - 1,
			(expected & PF_PAGED ? "p" : "-"),
			(expected & PF_COW ? "C" : "-"),
			(expected & PF_USER ? "U" : "-"),
			(expected & PF_WRITE ? "W" : "-")
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
	//__asm__ __volatile__ ("xchg %bx,%bx");
	// Check if the directory is mapped
	if( gaPageDir[ VAddr >> 22 ] == 0 )
	{
		// Allocate directory
		paddr = MM_AllocPhys();
		//LOG("paddr = 0x%llx (new table)", paddr);
		if( paddr == 0 ) {
			Warning("MM_Allocate - Out of Memory (Called by %p)", __builtin_return_address(0));
			//LEAVE('i',0);
			return 0;
		}
		// Map
		gaPageDir[ VAddr >> 22 ] = paddr | 3;
		// Mark as user
		if(VAddr < MM_USER_MAX)	gaPageDir[ VAddr >> 22 ] |= PF_USER;
		
		INVLPG( &gaPageDir[ VAddr >> 22 ] );
		//LOG("Clearing new table");
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
tPAddr MM_GetPhysAddr(tVAddr Addr)
{
	if( !(gaPageDir[Addr >> 22] & 1) )
		return 0;
	if( !(gaPageTable[Addr >> 12] & 1) )
		return 0;
	return (gaPageTable[Addr >> 12] & ~0xFFF) | (Addr & 0xFFF);
}


/**
 * \fn int MM_IsUser(tVAddr VAddr)
 * \brief Checks if a page is user accessable
 */
int MM_IsUser(tVAddr VAddr)
{
	if( !(gaPageDir[VAddr >> 22] & 1) )
		return 0;
	if( !(gaPageTable[VAddr >> 12] & 1) )
		return 0;
	if( !(gaPageTable[VAddr >> 12] & PF_USER) )
		return 0;
	return 1;
}

/**
 * \fn void MM_SetCR3(tPAddr CR3)
 * \brief Sets the current process space
 */
void MM_SetCR3(tPAddr CR3)
{
	__asm__ __volatile__ ("mov %0, %%cr3"::"r"(CR3));
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
		Warning("MM_Map - Physical or Virtual Addresses are not aligned");
		//LEAVE('i', 0);
		return 0;
	}
	
	// Align addresses
	PAddr &= ~0xFFF;	VAddr &= ~0xFFF;
	
	// Check if the directory is mapped
	if( gaPageDir[ VAddr >> 22 ] == 0 )
	{
		gaPageDir[ VAddr >> 22 ] = MM_AllocPhys() | 3;
		
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
 * \fn tVAddr MM_ClearUser()
 * \brief Clear user's address space
 */
tVAddr MM_ClearUser()
{
	Uint	i, j;
	
	// Copy Directories
	for( i = 0; i < (MM_USER_MAX>>22); i ++ )
	{
		// Check if directory is not allocated
		if( !(gaPageDir[i] & PF_PRESENT) ) {
			gaPageDir[i] = 0;
			continue;
		}
		
		
		for( j = 0; j < 1024; j ++ )
		{
			if( gaPageTable[i*1024+j] & 1 )
				MM_DerefPhys( gaPageTable[i*1024+j] & ~0xFFF );
			gaPageTable[i*1024+j] = 0;
		}
		
		MM_DerefPhys( gaPageDir[i] & ~0xFFF );
		gaPageDir[i] = 0;
		INVLPG( &gaPageTable[i*1024] );
	}
	INVLPG( gaPageDir );
	
	return *gpPageCR3;
}

/**
 * \fn tPAddr MM_Clone()
 * \brief Clone the current address space
 */
tPAddr MM_Clone()
{
	Uint	i, j;
	tVAddr	ret;
	Uint	page = 0;
	tVAddr	kStackBase = Proc_GetCurThread()->KernelStack - KERNEL_STACK_SIZE;
	void	*tmp;
	
	LOCK( &gilTempFractal );
	
	// Create Directory Table
	*gpTmpCR3 = MM_AllocPhys() | 3;
	INVLPG( gaTmpDir );
	//LOG("Allocated Directory (%x)", *gpTmpCR3);
	memsetd( gaTmpDir, 0, 1024 );
	
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
	
	// Map in kernel tables (and make fractal mapping)
	for( i = 768; i < 1024; i ++ )
	{
		// Fractal
		if( i == (PAGE_TABLE_ADDR >> 22) ) {
			gaTmpDir[ PAGE_TABLE_ADDR >> 22 ] = *gpTmpCR3;
			continue;
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
	for(i = KERNEL_STACKS >> 22;
		i < KERNEL_STACKS_END >> 22;
		i ++ )
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
			if( ((i*1024+j)*4096 & ~(KERNEL_STACK_SIZE-1)) != kStackBase ) {
				gaTmpTable[i*1024+j] = 0;
				continue;
			}
			
			// Allocate page
			gaTmpTable[i*1024+j] = MM_AllocPhys() | 3;
			
			MM_RefPhys( gaTmpTable[i*1024+j] & ~0xFFF );
			
			tmp = (void *) MM_MapTemp( gaTmpTable[i*1024+j] & ~0xFFF );
			memcpy( tmp, (void *)( (i*1024+j)*0x1000 ), 0x1000 );
			MM_FreeTemp( (Uint)tmp );
		}
	}
	
	ret = *gpTmpCR3 & ~0xFFF;
	RELEASE( &gilTempFractal );
	
	//LEAVE('x', ret);
	return ret;
}

/**
 * \fn tVAddr MM_NewKStack()
 * \brief Create a new kernel stack
 */
tVAddr MM_NewKStack()
{
	tVAddr	base = KERNEL_STACKS;
	Uint	i;
	for(;base<KERNEL_STACKS_END;base+=KERNEL_STACK_SIZE)
	{
		if(MM_GetPhysAddr(base) != 0)	continue;
		for(i=0;i<KERNEL_STACK_SIZE;i+=0x1000) {
			MM_Allocate(base+i);
		}
		return base+KERNEL_STACK_SIZE;
	}
	Warning("MM_NewKStack - No address space left\n");
	return 0;
}

/**
 * \fn tVAddr MM_NewWorkerStack()
 * \brief Creates a new worker stack
 */
tVAddr MM_NewWorkerStack()
{
	Uint	esp, ebp;
	Uint	oldstack;
	Uint	base, addr;
	 int	i, j;
	Uint	*tmpPage;
	tPAddr	pages[WORKER_STACK_SIZE>>12];
	
	// Get the old ESP and EBP
	__asm__ __volatile__ ("mov %%esp, %0": "=r"(esp));
	__asm__ __volatile__ ("mov %%ebp, %0": "=r"(ebp));
	
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
	
	// Acquire the lock for the temp fractal mappings
	LOCK(&gilTempFractal);
	
	// Set the temp fractals to TID0's address space
	*gpTmpCR3 = ((Uint)gaInitPageDir - KERNEL_BASE) | 3;
	//Log(" MM_NewWorkerStack: *gpTmpCR3 = 0x%x", *gpTmpCR3);
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
		pages[ addr >> 12 ] = MM_AllocPhys();
		gaTmpTable[ (base + addr) >> 12 ] = pages[addr>>12] | 3;
	}
	*gpTmpCR3 = 0;
	// Release the temp mapping lock
	RELEASE(&gilTempFractal);
	
	// Copy the old stack
	oldstack = (esp + KERNEL_STACK_SIZE-1) & ~(KERNEL_STACK_SIZE-1);
	esp = oldstack - esp;	// ESP as an offset in the stack
	
	// Make `base` be the top of the stack
	base += WORKER_STACK_SIZE;
	
	i = (WORKER_STACK_SIZE>>12) - 1;
	// Copy the contents of the old stack to the new one, altering the addresses
	// `addr` is refering to bytes from the stack base (mem downwards)
	for(addr = 0; addr < esp; addr += 0x1000)
	{
		Uint	*stack = (Uint*)( oldstack-(addr+0x1000) );
		tmpPage = (void*)MM_MapTemp( pages[i] );
		// Copy old stack
		for(j = 0; j < 1024; j++)
		{
			// Possible Stack address?
			if(oldstack-esp < stack[j] && stack[j] < oldstack)
				tmpPage[j] = base - (oldstack - stack[j]);
			else	// Seems not, best leave it alone
				tmpPage[j] = stack[j];
		}
		MM_FreeTemp((tVAddr)tmpPage);
		i --;
	}
	
	//Log("MM_NewWorkerStack: RETURN 0x%x", base);
	return base;
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
		if( Flags & MM_PFLAG_RO )	*ent &= ~PF_WRITE;
		else 	*ent |= PF_WRITE;
	}
	
	// Kernel
	if( Mask & MM_PFLAG_KERNEL )
	{
		if( Flags & MM_PFLAG_KERNEL )	*ent &= ~PF_USER;
		else	*ent |= PF_USER;
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
}

/**
 * \fn tPAddr MM_DuplicatePage(tVAddr VAddr)
 * \brief Duplicates a virtual page to a physical one
 */
tPAddr MM_DuplicatePage(tVAddr VAddr)
{
	tPAddr	ret;
	Uint	temp;
	 int	wasRO = 0;
	
	//ENTER("xVAddr", VAddr);
	
	// Check if mapped
	if( !(gaPageDir  [VAddr >> 22] & PF_PRESENT) )	return 0;
	if( !(gaPageTable[VAddr >> 12] & PF_PRESENT) )	return 0;
	
	// Page Align
	VAddr &= ~0xFFF;
	
	// Allocate new page
	ret = MM_AllocPhys();
	
	// Write-lock the page (to keep data constistent), saving its R/W state
	wasRO = (gaPageTable[VAddr >> 12] & PF_WRITE ? 0 : 1);
	gaPageTable[VAddr >> 12] &= ~PF_WRITE;
	INVLPG( VAddr );
	
	// Copy Data
	temp = MM_MapTemp(ret);
	memcpy( (void*)temp, (void*)VAddr, 0x1000 );
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
tVAddr MM_MapTemp(tPAddr PAddr)
{
	 int	i;
	
	//ENTER("XPAddr", PAddr);
	
	PAddr &= ~0xFFF;
	
	//LOG("gilTempMappings = %i", gilTempMappings);
	
	for(;;)
	{
		LOCK( &gilTempMappings );
		
		for( i = 0; i < NUM_TEMP_PAGES; i ++ )
		{
			// Check if page used
			if(gaPageTable[ (TEMP_MAP_ADDR >> 12) + i ] & 1)	continue;
			// Mark as used
			gaPageTable[ (TEMP_MAP_ADDR >> 12) + i ] = PAddr | 3;
			INVLPG( TEMP_MAP_ADDR + (i << 12) );
			//LEAVE('p', TEMP_MAP_ADDR + (i << 12));
			RELEASE( &gilTempMappings );
			return TEMP_MAP_ADDR + (i << 12);
		}
		RELEASE( &gilTempMappings );
		Threads_Yield();
	}
}

/**
 * \fn void MM_FreeTemp(tVAddr PAddr)
 * \brief Free's a temp mapping
 */
void MM_FreeTemp(tVAddr VAddr)
{
	 int	i = VAddr >> 12;
	//ENTER("xVAddr", VAddr);
	
	if(i >= (TEMP_MAP_ADDR >> 12))
		gaPageTable[ i ] = 0;
	
	//LEAVE('-');
}

/**
 * \fn tVAddr MM_MapHWPages(tPAddr PAddr, Uint Number)
 * \brief Allocates a contigous number of pages
 */
tVAddr MM_MapHWPages(tPAddr PAddr, Uint Number)
{
	 int	i, j;
	
	PAddr &= ~0xFFF;
	
	// Scan List
	for( i = 0; i < NUM_HW_PAGES; i ++ )
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
			return HW_MAP_ADDR + (i<<12);
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
tVAddr MM_AllocDMA(int Pages, int MaxBits, tPAddr *PhysAddr)
{
	tPAddr	maxCheck = (1 << MaxBits);
	tPAddr	phys;
	tVAddr	ret;
	
	ENTER("iPages iMaxBits pPhysAddr", Pages, MaxBits, PhysAddr);
	
	// Sanity Check
	if(MaxBits < 12 || !PhysAddr) {
		LEAVE('i', 0);
		return 0;
	}
	
	// Bound
	if(MaxBits >= PHYS_BITS)	maxCheck = -1;
	
	// Fast Allocate
	if(Pages == 1 && MaxBits >= PHYS_BITS)
	{
		phys = MM_AllocPhys();
		*PhysAddr = phys;
		ret = MM_MapHWPages(phys, 1);
		if(ret == 0) {
			MM_DerefPhys(phys);
			LEAVE('i', 0);
			return 0;
		}
		LEAVE('x', ret);
		return ret;
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
	
	*PhysAddr = phys;
	LEAVE('x', ret);
	return ret;
}

/**
 * \fn void MM_UnmapHWPages(tVAddr VAddr, Uint Number)
 * \brief Unmap a hardware page
 */
void MM_UnmapHWPages(tVAddr VAddr, Uint Number)
{
	 int	i, j;
	// Sanity Check
	if(VAddr < HW_MAP_ADDR || VAddr-Number*0x1000 > HW_MAP_MAX)	return;
	
	i = VAddr >> 12;
	
	LOCK( &gilTempMappings );	// Temp and HW share a directory, so they share a lock
	
	for( j = 0; j < Number; j++ )
	{
		MM_DerefPhys( gaPageTable[ (HW_MAP_ADDR >> 12) + i + j ] );
		gaPageTable[ (HW_MAP_ADDR >> 12) + i + j ] = 0;
	}
	
	RELEASE( &gilTempMappings );
}

// --- EXPORTS ---
EXPORT(MM_GetPhysAddr);
EXPORT(MM_Map);
//EXPORT(MM_Unmap);
EXPORT(MM_MapHWPages);
EXPORT(MM_AllocDMA);
EXPORT(MM_UnmapHWPages);
