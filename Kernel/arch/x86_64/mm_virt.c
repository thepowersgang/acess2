/*
 * Acess2 x86_64 Port
 * 
 * Virtual Memory Manager
 */
#define DEBUG	0
#include <acess.h>
#include <mm_virt.h>
#include <threads_int.h>
#include <proc.h>

// === CONSTANTS ===
#define PHYS_BITS	52	// TODO: Move out

#define PML4_SHIFT	39
#define PDP_SHIFT	30
#define PDIR_SHIFT	21
#define PTAB_SHIFT	12

#define	PADDR_MASK	0x7FFFFFFF##FFFFF000
#define PAGE_MASK	(((Uint)1 << 36)-1)
#define TABLE_MASK	(((Uint)1 << 27)-1)
#define PDP_MASK	(((Uint)1 << 18)-1)
#define PML4_MASK	(((Uint)1 << 9)-1)

#define	PF_PRESENT	0x001
#define	PF_WRITE	0x002
#define	PF_USER		0x004
#define	PF_LARGE	0x000
#define	PF_COW		0x200
#define	PF_PAGED	0x400
#define	PF_NX		0x80000000##00000000

// === MACROS ===
#define PAGETABLE(idx)	(*((tPAddr*)MM_FRACTAL_BASE+((idx)&PAGE_MASK)))
#define PAGEDIR(idx)	PAGETABLE((MM_FRACTAL_BASE>>12)+((idx)&TABLE_MASK))
#define PAGEDIRPTR(idx)	PAGEDIR((MM_FRACTAL_BASE>>21)+((idx)&PDP_MASK))
#define PAGEMAPLVL4(idx)	PAGEDIRPTR((MM_FRACTAL_BASE>>30)+((idx)&PML4_MASK))

#define TMPCR3()	PAGEMAPLVL4(MM_TMPFRAC_BASE>>39)
#define TMPTABLE(idx)	(*((tPAddr*)MM_TMPFRAC_BASE+((idx)&PAGE_MASK)))
#define TMPDIR(idx)	PAGETABLE((MM_TMPFRAC_BASE>>12)+((idx)&TABLE_MASK))
#define TMPDIRPTR(idx)	PAGEDIR((MM_TMPFRAC_BASE>>21)+((idx)&PDP_MASK))
#define TMPMAPLVL4(idx)	PAGEDIRPTR((MM_TMPFRAC_BASE>>30)+((idx)&PML4_MASK))

#define INVLPG(__addr)	__asm__ __volatile__ ("invlpg (%0)"::"r"(__addr))
#define INVLPG_ALL()	__asm__ __volatile__ ("mov %cr3,%rax;\n\tmov %rax,%cr3;")
#define INVLPG_GLOBAL()	__asm__ __volatile__ ("mov %cr4,%rax;\n\txorl $0x80, %eax;\n\tmov %rax,%cr4;\n\txorl $0x80, %eax;\n\tmov %rax,%cr4")

// === CONSTS ===
//tPAddr	* const gaPageTable = MM_FRACTAL_BASE;

// === IMPORTS ===
extern void	Error_Backtrace(Uint IP, Uint BP);
extern tPAddr	gInitialPML4[512];

// === PROTOTYPES ===
void	MM_InitVirt(void);
//void	MM_FinishVirtualInit(void);
void	MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs);
void	MM_DumpTables(tVAddr Start, tVAddr End);
 int	MM_GetPageEntryPtr(tVAddr Addr, BOOL bTemp, BOOL bAllocate, BOOL bLargePage, tPAddr **Pointer);
 int	MM_MapEx(tVAddr VAddr, tPAddr PAddr, BOOL bTemp, BOOL bLarge);
// int	MM_Map(tVAddr VAddr, tPAddr PAddr);
void	MM_Unmap(tVAddr VAddr);
void	MM_ClearUser(void);
 int	MM_GetPageEntry(tVAddr Addr, tPAddr *Phys, Uint *Flags);

// === GLOBALS ===
tMutex	glMM_TempFractalLock;

// === CODE ===
void MM_InitVirt(void)
{
	MM_DumpTables(0, -1L);
}

void MM_FinishVirtualInit(void)
{
	PAGEMAPLVL4(0) = 0;
}

/**
 * \brief Called on a page fault
 */
void MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs)
{
	// TODO: Implement Copy-on-Write
	#if 0
	if( gaPageDir  [Addr>>22] & PF_PRESENT
	 && gaPageTable[Addr>>12] & PF_PRESENT
	 && gaPageTable[Addr>>12] & PF_COW )
	{
		tPAddr	paddr;
		if(MM_GetRefCount( gaPageTable[Addr>>12] & PADDR_MASK ) == 1)
		{
			gaPageTable[Addr>>12] &= ~PF_COW;
			gaPageTable[Addr>>12] |= PF_PRESENT|PF_WRITE;
		}
		else
		{
			//Log("MM_PageFault: COW - MM_DuplicatePage(0x%x)", Addr);
			paddr = MM_DuplicatePage( Addr );
			MM_DerefPhys( gaPageTable[Addr>>12] & PADDR_MASK );
			gaPageTable[Addr>>12] &= PF_USER;
			gaPageTable[Addr>>12] |= paddr|PF_PRESENT|PF_WRITE;
		}
		
		INVLPG( Addr & ~0xFFF );
		return;
	}
	#endif
	
	// If it was a user, tell the thread handler
	if(ErrorCode & 4) {
		Warning("%s %s %s memory%s",
			(ErrorCode&4?"User":"Kernel"),
			(ErrorCode&2?"write to":"read from"),
			(ErrorCode&1?"bad/locked":"non-present"),
			(ErrorCode&16?" (Instruction Fetch)":"")
			);
		Warning("User Pagefault: Instruction at %04x:%08x accessed %p",
			Regs->CS, Regs->RIP, Addr);
		__asm__ __volatile__ ("sti");	// Restart IRQs
//		Threads_SegFault(Addr);
		return ;
	}
	
	// Kernel #PF
	Debug_KernelPanic();
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
	
	Log("Code at %p accessed %p", Regs->RIP, Addr);
	// Print Stack Backtrace
	Error_Backtrace(Regs->RIP, Regs->RBP);
	
	MM_DumpTables(0, -1);
	
	__asm__ __volatile__ ("cli");
	for( ;; )
		HALT();
}

/**
 * \brief Dumps the layout of the page tables
 */
void MM_DumpTables(tVAddr Start, tVAddr End)
{
	#define CANOICAL(addr)	((addr)&0x800000000000?(addr)|0xFFFF000000000000:(addr))
	const tPAddr	CHANGEABLE_BITS = 0xFF8;
	const tPAddr	MASK = ~CHANGEABLE_BITS;	// Physical address and access bits
	tVAddr	rangeStart = 0;
	tPAddr	expected = CHANGEABLE_BITS;	// CHANGEABLE_BITS is used because it's not a vaild value
	tVAddr	curPos;
	Uint	page;
	
	Log("Table Entries: (%p to %p)", Start, End);
	
	End &= (1L << 48) - 1;
	
	Start >>= 12;	End >>= 12;
	
	for(page = Start, curPos = Start<<12;
		page < End;
		curPos += 0x1000, page++)
	{
		if( curPos == 0x800000000000L )
			curPos = 0xFFFF800000000000L;
		
		//Debug("&PAGEMAPLVL4(%i page>>27) = %p", page>>27, &PAGEMAPLVL4(page>>27));
		//Debug("&PAGEDIRPTR(%i page>>18) = %p", page>>18, &PAGEDIRPTR(page>>18));
		//Debug("&PAGEDIR(%i page>>9) = %p", page>>9, &PAGEDIR(page>>9));
		//Debug("&PAGETABLE(%i page) = %p", page, &PAGETABLE(page));
		
		// End of a range
		if(
			!(PAGEMAPLVL4(page>>27) & PF_PRESENT)
		||	!(PAGEDIRPTR(page>>18) & PF_PRESENT)
		||	!(PAGEDIR(page>>9) & PF_PRESENT)
		||  !(PAGETABLE(page) & PF_PRESENT)
		||  (PAGETABLE(page) & MASK) != expected)
		{			
			if(expected != CHANGEABLE_BITS) {
				Log("%016llx => %013llx : 0x%6llx (%c%c%c%c)",
					CANOICAL(rangeStart),
					PAGETABLE(rangeStart>>12) & PADDR_MASK,
					curPos - rangeStart,
					(expected & PF_PAGED ? 'p' : '-'),
					(expected & PF_COW ? 'C' : '-'),
					(expected & PF_USER ? 'U' : '-'),
					(expected & PF_WRITE ? 'W' : '-')
					);
				expected = CHANGEABLE_BITS;
			}
			if( !(PAGEMAPLVL4(page>>27) & PF_PRESENT) ) {
				page += (1 << 27) - 1;
				curPos += (1L << 39) - 0x1000;
				//Debug("pml4 ent unset (page = 0x%x now)", page);
				continue;
			}
			if( !(PAGEDIRPTR(page>>18) & PF_PRESENT) ) {
				page += (1 << 18) - 1;
				curPos += (1L << 30) - 0x1000;
				//Debug("pdp ent unset (page = 0x%x now)", page);
				continue;
			}
			if( !(PAGEDIR(page>>9) & PF_PRESENT) ) {
				page += (1 << 9) - 1;
				curPos += (1L << 21) - 0x1000;
				//Debug("pd ent unset (page = 0x%x now)", page);
				continue;
			}
			if( !(PAGETABLE(page) & PF_PRESENT) )	continue;
			
			expected = (PAGETABLE(page) & MASK);
			rangeStart = curPos;
		}
		if(expected != CHANGEABLE_BITS)
			expected += 0x1000;
	}
	
	if(expected != CHANGEABLE_BITS) {
		Log("%016llx => %013llx : 0x%6llx (%c%c%c%c)",
			CANOICAL(rangeStart),
			PAGETABLE(rangeStart>>12) & PADDR_MASK,
			curPos - rangeStart,
			(expected & PF_PAGED ? 'p' : '-'),
			(expected & PF_COW ? 'C' : '-'),
			(expected & PF_USER ? 'U' : '-'),
			(expected & PF_WRITE ? 'W' : '-')
			);
		expected = 0;
	}
	#undef CANOICAL
}

/**
 * \brief Get a pointer to a page entry
 * \param Addr	Virtual Address
 * \param bTemp	Use the Temporary fractal mapping
 * \param bAllocate	Allocate entries
 * \param bLargePage	Request a large page
 * \param Pointer	Location to place the calculated pointer
 * \return Page size, or -ve on error
 */
int MM_GetPageEntryPtr(tVAddr Addr, BOOL bTemp, BOOL bAllocate, BOOL bLargePage, tPAddr **Pointer)
{
	tPAddr	*pmlevels[4];
	tPAddr	tmp;
	const int	ADDR_SIZES[] = {39, 30, 21, 12};
	const int	nADDR_SIZES = sizeof(ADDR_SIZES)/sizeof(ADDR_SIZES[0]);
	 int	i;
	
	if( bTemp )
	{
		pmlevels[3] = &TMPTABLE(0);	// Page Table
		pmlevels[2] = &TMPDIR(0);	// PDIR
		pmlevels[1] = &TMPDIRPTR(0);	// PDPT
		pmlevels[0] = &TMPMAPLVL4(0);	// PML4
	}
	else
	{
		pmlevels[3] = (void*)MM_FRACTAL_BASE;	// Page Table
		pmlevels[2] = &pmlevels[3][(MM_FRACTAL_BASE>>12)&PAGE_MASK];	// PDIR
		pmlevels[1] = &pmlevels[2][(MM_FRACTAL_BASE>>21)&TABLE_MASK];	// PDPT
		pmlevels[0] = &pmlevels[1][(MM_FRACTAL_BASE>>30)&PDP_MASK];	// PML4
	}
	
	// Mask address
	Addr &= (1ULL << 48)-1;
	
	for( i = 0; i < nADDR_SIZES-1; i ++ )
	{
//		INVLPG( &pmlevels[i][ (Addr >> ADDR_SIZES[i]) & 
		
		// Check for a large page
		if( (Addr & ((1ULL << ADDR_SIZES[i])-1)) == 0 && bLargePage )
		{
			if(Pointer)	*Pointer = &pmlevels[i][Addr >> ADDR_SIZES[i]];
			return ADDR_SIZES[i];
		}
		// Allocate an entry if required
		if( !(pmlevels[i][Addr >> ADDR_SIZES[i]] & 1) )
		{
			if( !bAllocate )	return -4;	// If allocation is not requested, error
			tmp = MM_AllocPhys();
			if(!tmp)	return -2;
			pmlevels[i][Addr >> ADDR_SIZES[i]] = tmp | 3;
			INVLPG( &pmlevels[i+1][ (Addr>>ADDR_SIZES[i])*512 ] );
			memset( &pmlevels[i+1][ (Addr>>ADDR_SIZES[i])*512 ], 0, 0x1000 );
		}
		// Catch large pages
		else if( pmlevels[i][Addr >> ADDR_SIZES[i]] & PF_LARGE )
		{
			// Alignment
			if( (Addr & ((1ULL << ADDR_SIZES[i])-1)) != 0 )	return -3;
			if(Pointer)	*Pointer = &pmlevels[i][Addr >> ADDR_SIZES[i]];
			return ADDR_SIZES[i];	// Large page warning
		}
	}
	
	// And, set the page table entry
	if(Pointer)	*Pointer = &pmlevels[i][Addr >> ADDR_SIZES[i]];
	return ADDR_SIZES[i];
}

/**
 * \brief Map a physical page to a virtual one
 * \param VAddr	Target virtual address
 * \param PAddr	Physical address of page
 * \param bTemp	Use tempoary mappings
 * \param bLarge	Treat as a large page
 */
int MM_MapEx(tVAddr VAddr, tPAddr PAddr, BOOL bTemp, BOOL bLarge)
{
	tPAddr	*ent;
	 int	rv;
	
	ENTER("xVAddr xPAddr", VAddr, PAddr);
	
	// Get page pointer (Allow allocating)
	rv = MM_GetPageEntryPtr(VAddr, bTemp, 1, bLarge, &ent);
	if(rv < 0)	LEAVE_RET('i', 0);
	
	if( *ent & 1 )	LEAVE_RET('i', 0);
	
	*ent = PAddr | 3;
	
	INVLPG( VAddr );

	LEAVE('i', 1);	
	return 1;
}

/**
 * \brief Map a physical page to a virtual one
 * \param VAddr	Target virtual address
 * \param PAddr	Physical address of page
 */
int MM_Map(tVAddr VAddr, tPAddr PAddr)
{
	return MM_MapEx(VAddr, PAddr, 0, 0);
}

/**
 * \brief Removed a mapped page
 */
void MM_Unmap(tVAddr VAddr)
{
	// Check PML4
	if( !(PAGEMAPLVL4(VAddr >> 39) & 1) )	return ;
	// Check PDP
	if( !(PAGEDIRPTR(VAddr >> 30) & 1) )	return ;
	// Check Page Dir
	if( !(PAGEDIR(VAddr >> 21) & 1) )	return ;
	
	PAGETABLE(VAddr >> PTAB_SHIFT) = 0;
	INVLPG( VAddr );
}

/**
 * \brief Allocate a block of memory at the specified virtual address
 */
tPAddr MM_Allocate(tVAddr VAddr)
{
	tPAddr	ret;
	
	ENTER("xVAddr", VAddr);
	
	// Ensure the tables are allocated before the page (keeps things neat)
	MM_GetPageEntryPtr(VAddr, 0, 1, 0, NULL);
	
	// Allocate the page
	ret = MM_AllocPhys();
	LOG("ret = %x", ret);
	if(!ret)	LEAVE_RET('i', 0);
	
	if( !MM_Map(VAddr, ret) )
	{
		Warning("MM_Allocate: Unable to map. Strange, we should have errored earlier");
		MM_DerefPhys(ret);
		LEAVE('i');
		return 0;
	}
	
	LEAVE('X', ret);
	return ret;
}

/**
 * \brief Deallocate a page at a virtual address
 */
void MM_Deallocate(tVAddr VAddr)
{
	tPAddr	phys;
	
	phys = MM_GetPhysAddr(VAddr);
	if(!phys)	return ;
	
	MM_Unmap(VAddr);
	
	MM_DerefPhys(phys);
}

/**
 * \brief Get the page table entry of a virtual address
 * \param Addr	Virtual Address
 * \param Phys	Location to put the physical address
 * \param Flags	Flags on the entry (set to zero if unmapped)
 * \return Size of the entry (in address bits) - 12 = 4KiB page
 */
int MM_GetPageEntry(tVAddr Addr, tPAddr *Phys, Uint *Flags)
{
	tPAddr	*ptr;
	 int	ret;
	
	if(!Phys || !Flags)	return 0;
	
	ret = MM_GetPageEntryPtr(Addr, 0, 0, 0, &ptr);
	if( ret < 0 )	return 0;
	
	*Phys = *ptr & PADDR_MASK;
	*Flags = *ptr & 0xFFF;
	return ret;
}

/**
 * \brief Get the physical address of a virtual location
 */
tPAddr MM_GetPhysAddr(tVAddr Addr)
{
	tPAddr	*ptr;
	 int	ret;
	
	ret = MM_GetPageEntryPtr(Addr, 0, 0, 0, &ptr);
	if( ret < 0 )	return 0;
	
	return (*ptr & PADDR_MASK) | (Addr & 0xFFF);
}

/**
 * \brief Sets the flags on a page
 */
void MM_SetFlags(tVAddr VAddr, Uint Flags, Uint Mask)
{
	tPAddr	*ent;
	 int	rv;
	
	// Get pointer
	rv = MM_GetPageEntryPtr(VAddr, 0, 0, 0, &ent);
	if(rv < 0)	return ;
	
	// Ensure the entry is valid
	if( !(*ent & 1) )	return ;
	
	// Read-Only
	if( Mask & MM_PFLAG_RO )
	{
		if( Flags & MM_PFLAG_RO ) {
			*ent &= ~PF_WRITE;
		}
		else {
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
	
	// Execute
	if( Mask & MM_PFLAG_EXEC )
	{
		if( Flags & MM_PFLAG_EXEC ) {
			*ent &= ~PF_NX;
		}
		else {
			*ent |= PF_NX;
		}
	}
}

/**
 * \brief Get the flags applied to a page
 */
Uint MM_GetFlags(tVAddr VAddr)
{
	tPAddr	*ent;
	 int	rv, ret = 0;
	
	rv = MM_GetPageEntryPtr(VAddr, 0, 0, 0, &ent);
	if(rv < 0)	return 0;
	
	if( !(*ent & 1) )	return 0;
	
	// Read-Only
	if( !(*ent & PF_WRITE) )	ret |= MM_PFLAG_RO;
	// Kernel
	if( !(*ent & PF_USER) )	ret |= MM_PFLAG_KERNEL;
	// Copy-On-Write
	if( *ent & PF_COW )	ret |= MM_PFLAG_COW;	
	// Execute
	if( !(*ent & PF_NX) )	ret |= MM_PFLAG_EXEC;
	
	return ret;
}

// --- Hardware Mappings ---
/**
 * \brief Map a range of hardware pages
 */
tVAddr MM_MapHWPages(tPAddr PAddr, Uint Number)
{
	tVAddr	ret;
	 int	num;
	
	//TODO: Add speedups (memory of first possible free)
	for( ret = MM_HWMAP_BASE; ret < MM_HWMAP_TOP; ret += 0x1000 )
	{
		for( num = Number; num -- && ret < MM_HWMAP_TOP; ret += 0x1000 )
		{
			if( MM_GetPhysAddr(ret) != 0 )	break;
		}
		if( num >= 0 )	continue;
		
		PAddr += 0x1000 * Number;
		
		while( Number -- )
		{
			ret -= 0x1000;
			PAddr -= 0x1000;
			MM_Map(ret, PAddr);
		}
		
		return ret;
	}
	
	Log_KernelPanic("MM", "TODO: Implement MM_MapHWPages");
	return 0;
}

/**
 * \brief Free a range of hardware pages
 */
void MM_UnmapHWPages(tVAddr VAddr, Uint Number)
{
//	Log_KernelPanic("MM", "TODO: Implement MM_UnmapHWPages");
	while( Number -- )
	{
		MM_Unmap(VAddr);
		VAddr += 0x1000;
	}
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
	tPAddr	phys;
	tVAddr	ret;
	
	// Sanity Check
	if(MaxBits < 12 || !PhysAddr)	return 0;
	
	// Fast Allocate
	if(Pages == 1 && MaxBits >= PHYS_BITS)
	{
		phys = MM_AllocPhys();
		*PhysAddr = phys;
		ret = MM_MapHWPages(phys, 1);
		if(ret == 0) {
			MM_DerefPhys(phys);
			return 0;
		}
		return ret;
	}
	
	// Slow Allocate
	phys = MM_AllocPhysRange(Pages, MaxBits);
	// - Was it allocated?
	if(phys == 0)	return 0;
	
	// Allocated successfully, now map
	ret = MM_MapHWPages(phys, Pages);
	if( ret == 0 ) {
		// If it didn't map, free then return 0
		for(;Pages--;phys+=0x1000)
			MM_DerefPhys(phys);
		return 0;
	}
	
	*PhysAddr = phys;
	return ret;
}

// --- Tempory Mappings ---
tVAddr MM_MapTemp(tPAddr PAddr)
{
	const int max_slots = (MM_TMPMAP_END - MM_TMPMAP_BASE) / PAGE_SIZE;
	tVAddr	ret = MM_TMPMAP_BASE;
	 int	i;
	
	for( i = 0; i < max_slots; i ++, ret += PAGE_SIZE )
	{
		tPAddr	*ent;
		if( MM_GetPageEntryPtr( ret, 0, 1, 0, &ent) < 0 ) {
			continue ;
		}

		if( *ent & 1 )
			continue ;

		*ent = PAddr | 3;
		return ret;
	}
	return 0;
}

void MM_FreeTemp(tVAddr VAddr)
{
	MM_Deallocate(VAddr);
	return ;
}


// --- Address Space Clone --
tPAddr MM_Clone(void)
{
	tPAddr	ret;
	 int	i;
	tVAddr	kstackbase;

	// tThread->KernelStack is the top
	// There is 1 guard page below the stack
	kstackbase = Proc_GetCurThread()->KernelStack - KERNEL_STACK_SIZE + 0x1000;

	Log("MM_Clone: kstackbase = %p", kstackbase);
	
	// #1 Create a copy of the PML4
	ret = MM_AllocPhys();
	if(!ret)	return 0;
	
	// #2 Alter the fractal pointer
	Mutex_Acquire(&glMM_TempFractalLock);
	TMPCR3() = ret | 3;
	INVLPG_ALL();
	
	// #3 Set Copy-On-Write to all user pages
	for( i = 0; i < 256; i ++)
	{
		TMPMAPLVL4(i) = PAGEMAPLVL4(i);
//		Log_Debug("MM", "TMPMAPLVL4(%i) = 0x%016llx", i, TMPMAPLVL4(i));
		if( TMPMAPLVL4(i) & 1 )
		{
			MM_RefPhys( TMPMAPLVL4(i) & PADDR_MASK );
			TMPMAPLVL4(i) |= PF_COW;
			TMPMAPLVL4(i) &= ~PF_WRITE;
		}
	}
	
	// #4 Map in kernel pages
	for( i = 256; i < 512; i ++ )
	{
		// Skip addresses:
		// 320 0xFFFFA....	- Kernel Stacks
		if( i == 320 )	continue;
		// 509 0xFFFFFE0..	- Fractal mapping
		if( i == 509 )	continue;
		// 510 0xFFFFFE8..	- Temp fractal mapping
		if( i == 510 )	continue;
		
		TMPMAPLVL4(i) = PAGEMAPLVL4(i);
		if( TMPMAPLVL4(i) & 1 )
			MM_RefPhys( TMPMAPLVL4(i) & PADDR_MASK );
	}
	
	// #5 Set fractal mapping
	TMPMAPLVL4(509) = ret | 3;
	TMPMAPLVL4(510) = 0;	// Temp
	
	// #6 Create kernel stack (-1 to account for the guard)
	TMPMAPLVL4(320) = 0;
	for( i = 0; i < KERNEL_STACK_SIZE/0x1000-1; i ++ )
	{
		tPAddr	phys = MM_AllocPhys();
		tVAddr	tmpmapping;
		MM_MapEx(kstackbase+i*0x1000, phys, 1, 0);
		
		tmpmapping = MM_MapTemp(phys);
		memcpy((void*)tmpmapping, (void*)(kstackbase+i*0x1000), 0x1000);
		MM_FreeTemp(tmpmapping);
	}
	
	// #7 Return
	TMPCR3() = 0;
	INVLPG_ALL();
	Mutex_Release(&glMM_TempFractalLock);
	Log("MM_Clone: RETURN %P\n", ret);
	return ret;
}

void MM_ClearUser(void)
{
	tVAddr	addr = 0;
	 int	pml4, pdpt, pd, pt;
	
	for( pml4 = 0; pml4 < 256; pml4 ++ )
	{
		// Catch an un-allocated PML4 entry
		if( !(PAGEMAPLVL4(pml4) & 1) ) {
			addr += 1ULL << PML4_SHIFT;
			continue ;
		}
		
		// Catch a large COW
		if( (PAGEMAPLVL4(pml4) & PF_COW) ) {
			addr += 1ULL << PML4_SHIFT;
		}
		else
		{
			// TODO: Large pages
			
			// Child entries
			for( pdpt = 0; pdpt < 512; pdpt ++ )
			{
				// Unallocated
				if( !(PAGEDIRPTR(addr >> PDP_SHIFT) & 1) ) {
					addr += 1ULL << PDP_SHIFT;
					continue;
				}
			
				// Catch a large COW
				if( (PAGEDIRPTR(addr >> PDP_SHIFT) & PF_COW) ) {
					addr += 1ULL << PDP_SHIFT;
				}
				else {
					// Child entries
					for( pd = 0; pd < 512; pd ++ )
					{
						// Unallocated PDir entry
						if( !(PAGEDIR(addr >> PDIR_SHIFT) & 1) ) {
							addr += 1ULL << PDIR_SHIFT;
							continue;
						}
						
						// COW Page Table
						if( PAGEDIR(addr >> PDIR_SHIFT) & PF_COW ) {
							addr += 1ULL << PDIR_SHIFT;
						}
						else
						{
							// TODO: Catch large pages
							
							// Child entries
							for( pt = 0; pt < 512; pt ++ )
							{
								// Free page
								if( PAGETABLE(addr >> PTAB_SHIFT) & 1 ) {
									MM_DerefPhys( PAGETABLE(addr >> PTAB_SHIFT) & PADDR_MASK );
									PAGETABLE(addr >> PTAB_SHIFT) = 0;
								}
								addr += 1ULL << 12;
							}
						}
						// Free page table
						MM_DerefPhys( PAGEDIR(addr >> PDIR_SHIFT) & PADDR_MASK );
						PAGEDIR(addr >> PDIR_SHIFT) = 0;
					}
				}
				// Free page directory
				MM_DerefPhys( PAGEDIRPTR(addr >> PDP_SHIFT) & PADDR_MASK );
				PAGEDIRPTR(addr >> PDP_SHIFT) = 0;
			}
		}
		// Free page directory pointer table (PML4 entry)
		MM_DerefPhys( PAGEMAPLVL4(pml4) & PADDR_MASK );
		PAGEMAPLVL4(pml4) = 0;
	}
}

tVAddr MM_NewWorkerStack(void)
{
	tVAddr	ret;
	 int	i;
	
	// #1 Set temp fractal to PID0
	Mutex_Acquire(&glMM_TempFractalLock);
	TMPCR3() = ((tPAddr)gInitialPML4 - KERNEL_BASE) | 3;
	
	// #2 Scan for a free stack addresss < 2^47
	for(ret = 0x100000; ret < (1ULL << 47); ret += KERNEL_STACK_SIZE)
	{
		if( MM_GetPhysAddr(ret) == 0 )	break;
	}
	if( ret >= (1ULL << 47) ) {
		Mutex_Release(&glMM_TempFractalLock);
		return 0;
	}
	
	// #3 Map all save the last page in the range
	//    - This acts as as guard page, and doesn't cost us anything.
	for( i = 0; i < KERNEL_STACK_SIZE/0x1000 - 1; i ++ )
	{
		tPAddr	phys = MM_AllocPhys();
		if(!phys) {
			// TODO: Clean up
			Log_Error("MM", "MM_NewWorkerStack - Unable to allocate page");
			return 0;
		}
		MM_MapEx(ret + i*0x1000, phys, 1, 0);
	}
	
	Mutex_Release(&glMM_TempFractalLock);
	
	return ret + i*0x1000;
}

/**
 * \brief Allocate a new kernel stack
 */
tVAddr MM_NewKStack(void)
{
	tVAddr	base = MM_KSTACK_BASE;
	Uint	i;
	for( ; base < MM_KSTACK_TOP; base += KERNEL_STACK_SIZE )
	{
		if(MM_GetPhysAddr(base) != 0)
			continue;
		
		//Log("MM_NewKStack: Found one at %p", base + KERNEL_STACK_SIZE);
		for( i = 0; i < KERNEL_STACK_SIZE; i += 0x1000)
		{
			if( !MM_Allocate(base+i) )
			{
				Log_Warning("MM", "MM_NewKStack - Allocation failed");
				for( i -= 0x1000; i; i -= 0x1000)
					MM_Deallocate(base+i);
				return 0;
			}
		}
		
		return base + KERNEL_STACK_SIZE;
	}
	Log_Warning("MM", "MM_NewKStack - No address space left\n");
	return 0;
}
