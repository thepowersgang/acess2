/*
 * Acess2 x86_64 Port
 * 
 * Virtual Memory Manager
 */
#define DEBUG	0
#include <acess.h>
#include <mm_virt.h>
#include <proc.h>

// === CONSTANTS ===
#define PML4_SHIFT	39
#define PDP_SHIFT	30
#define PDIR_SHIFT	21
#define PTAB_SHIFT	12

#define	PADDR_MASK	0x7FFFFFFF##FFFFF000
#define PAGE_MASK	(((Uint)1 << 36)-1)
#define TABLE_MASK	(((Uint)1 << 27)-1)
#define PDP_MASK	(((Uint)1 << 18)-1)
#define PML4_MASK	(((Uint)1 << 9)-1)

#define	PF_PRESENT	0x1
#define	PF_WRITE	0x2
#define	PF_USER		0x4
#define	PF_LARGE	0x0
#define	PF_COW		0x200
#define	PF_PAGED	0x400
#define	PF_NX		0x80000000##00000000

// === MACROS ===
#define PAGETABLE(idx)	(*((tPAddr*)MM_FRACTAL_BASE+((idx)&PAGE_MASK)))
#define PAGEDIR(idx)	PAGETABLE((MM_FRACTAL_BASE>>12)+((idx)&TABLE_MASK))
#define PAGEDIRPTR(idx)	PAGEDIR((MM_FRACTAL_BASE>>21)+((idx)&PDP_MASK))
#define PAGEMAPLVL4(idx)	PAGEDIRPTR((MM_FRACTAL_BASE>>30)+((idx)&PML4_MASK))

#define INVLPG(__addr)	__asm__ __volatile__ ("invlpg (%0)"::"r"(__addr));

// === PROTOTYPES ===
void	MM_InitVirt(void);
//void	MM_FinishVirtualInit(void);
void	MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs);
void	MM_DumpTables(tVAddr Start, tVAddr End);
// int	MM_Map(tVAddr VAddr, tPAddr PAddr);
void	MM_Unmap(tVAddr VAddr);
void	MM_ClearUser(void);
 int	MM_GetPageEntry(tVAddr Addr, tPAddr *Phys, Uint *Flags);

// === GLOBALS ===

// === CODE ===
void MM_InitVirt(void)
{
	MM_DumpTables(0, -1L);
}

void MM_FinishVirtualInit(void)
{
}

/**
 * \brief Called on a page fault
 */
void MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs)
{
	// TODO: Copy on Write
	#if 0
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
//	Error_Backtrace(Regs->RIP, Regs->RBP);
	
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
	const tPAddr	CHANGEABLE_BITS = 0xFF8;
	const tPAddr	MASK = ~CHANGEABLE_BITS;	// Physical address and access bits
	tVAddr	rangeStart = 0;
	tPAddr	expected = CHANGEABLE_BITS;	// MASK is used because it's not a vaild value
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
				Log("%016x-0x%016x => %013x-%013x (%c%c%c%c)",
					rangeStart, curPos - 1,
					PAGETABLE(rangeStart>>12) & ~0xFFF,
					(expected & ~0xFFF) - 1,
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
		Log("%016x-%016x => %013x-%013x (%s%s%s%s)",
			rangeStart, curPos - 1,
			PAGETABLE(rangeStart>>12) & ~0xFFF,
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
 * \brief Map a physical page to a virtual one
 */
int MM_Map(tVAddr VAddr, tPAddr PAddr)
{
	tPAddr	tmp;
	
	ENTER("xVAddr xPAddr", VAddr, PAddr);
	
	// Check that the page hasn't been mapped already
	{
		Uint	flags;
		 int	ret;
		ret = MM_GetPageEntry(VAddr, &tmp, &flags);
		if( flags & PF_PRESENT ) {
			LEAVE('i', 0);
			return 0;
		}
	}
	
	// Check PML4
	if( !(PAGEMAPLVL4(VAddr >> 39) & 1) )
	{
		tmp = MM_AllocPhys();
		if(!tmp) {
			LEAVE('i', 0);
			return 0;
		}
		PAGEMAPLVL4(VAddr >> 39) = tmp | 3;
		INVLPG( &PAGEDIRPTR( (VAddr>>39)<<9 ) );
		memset( &PAGEDIRPTR( (VAddr>>39)<<9 ), 0, 4096 );
	}
	
	// Check PDP
	if( !(PAGEDIRPTR(VAddr >> 30) & 1) )
	{
		tmp = MM_AllocPhys();
		if(!tmp) {
			LEAVE('i', 0);
			return 0;
		}
		PAGEDIRPTR(VAddr >> 30) = tmp | 3;
		INVLPG( &PAGEDIR( (VAddr>>30)<<9 ) );
		memset( &PAGEDIR( (VAddr>>30)<<9 ), 0, 0x1000 );
	}
	
	// Check Page Dir
	if( !(PAGEDIR(VAddr >> 21) & 1) )
	{
		tmp = MM_AllocPhys();
		if(!tmp) {
			LEAVE('i', 0);
			return 0;
		}
		PAGEDIR(VAddr >> 21) = tmp | 3;
		INVLPG( &PAGETABLE( (VAddr>>21)<<9 ) );
		memset( &PAGETABLE( (VAddr>>21)<<9 ), 0, 4096 );
	}
	
	// Check if this virtual address is already mapped
	if( PAGETABLE(VAddr >> PTAB_SHIFT) & 1 ) {
		LEAVE('i', 0);
		return 0;
	}
	
	PAGETABLE(VAddr >> PTAB_SHIFT) = PAddr | 3;
	
	INVLPG( VAddr );

	LEAVE('i', 1);	
	return 1;
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
	
	// NOTE: This is hack, but I like my dumps to be neat
	#if 1
	if( !MM_Map(VAddr, 0) )	// Make sure things are allocated
	{
		Warning("MM_Allocate: Unable to map, tables did not initialise");
		LEAVE('i', 0);
		return 0;
	}
	MM_Unmap(VAddr);
	#endif
	
	ret = MM_AllocPhys();
	LOG("ret = %x", ret);
	if(!ret) {
		LEAVE('i', 0);
		return 0;
	}
	
	if( !MM_Map(VAddr, ret) )
	{
		Warning("MM_Allocate: Unable to map. Strange, we should have errored earlier");
		MM_DerefPhys(ret);
		LEAVE('i');
		return 0;
	}
	
	LEAVE('x', ret);
	return ret;
}

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
	if(!Phys || !Flags)	return 0;
	
	// Check if the PML4 entry is present
	if( !(PAGEMAPLVL4(Addr >> 39) & 1) ) {
		*Phys = 0;
		*Flags = 0;
		return 39;
	}
	// - Check for large page
	if( PAGEMAPLVL4(Addr >> 39) & PF_LARGE ) {
		*Phys = PAGEMAPLVL4(Addr >> 39) & ~0xFFF;
		*Flags = PAGEMAPLVL4(Addr >> 39) & 0xFFF;
		return 39;
	}
	
	// Check the PDP entry
	if( !(PAGEDIRPTR(Addr >> 30) & 1) ) {
		*Phys = 0;
		*Flags = 0;
		return 30;
	}
	// - Check for large page
	if( PAGEDIRPTR(Addr >> 30) & PF_LARGE ) {
		*Phys = PAGEDIRPTR(Addr >> 30) & ~0xFFF;
		*Flags = PAGEDIRPTR(Addr >> 30) & 0xFFF;
		return 30;
	}
	
	// Check PDIR Entry
	if( !(PAGEDIR(Addr >> 21) & 1) ) {
		*Phys = 0;
		*Flags = 0;
		return 21;
	}
	// - Check for large page
	if( PAGEDIR(Addr >> 21) & PF_LARGE ) {
		*Phys = PAGEDIR(Addr >> 21) & ~0xFFF;
		*Flags = PAGEDIR(Addr >> 21) & 0xFFF;
		return 21;
	}
	
	// And, check the page table entry
	if( !(PAGETABLE(Addr >> PTAB_SHIFT) & 1) ) {
		*Phys = 0;
		*Flags = 0;
	}
	else {
		*Phys = PAGETABLE(Addr >> PTAB_SHIFT) & ~0xFFF;
		*Flags = PAGETABLE(Addr >> PTAB_SHIFT) & 0xFFF;
	}
	return 12;
}

/**
 * \brief Get the physical address of a virtual location
 */
tPAddr MM_GetPhysAddr(tVAddr Addr)
{
	tPAddr	ret;
	Uint	flags;
	
	MM_GetPageEntry(Addr, &ret, &flags);
	
	return ret | (Addr & 0xFFF);
}

/**
 * \brief Sets the flags on a page
 */
void MM_SetFlags(tVAddr VAddr, Uint Flags, Uint Mask)
{
	tPAddr	*ent;
	
	// Validity Check
	if( !(PAGEMAPLVL4(VAddr >> 39) & 1) )
		return ;
	if( !(PAGEDIRPTR(VAddr >> 30) & 1) )
		return ;
	if( !(PAGEDIR(VAddr >> 21) & 1) )
		return ;
	if( !(PAGETABLE(VAddr >> 12) & 1) )
		return ;
	
	// Set Flags
	ent = &PAGETABLE(VAddr >> 12);
	
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
	Uint	ret = 0;
	
	// Validity Check
	if( !(PAGEMAPLVL4(VAddr >> 39) & 1) )
		return 0;
	if( !(PAGEDIRPTR(VAddr >> 30) & 1) )
		return 0;
	if( !(PAGEDIR(VAddr >> 21) & 1) )
		return 0;
	if( !(PAGETABLE(VAddr >> 12) & 1) )
		return 0;
	
	// Set Flags
	ent = &PAGETABLE(VAddr >> 12);
	
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
	Log_KernelPanic("MM", "TODO: Implement MM_MapHWPages");
	return 0;
}

/**
 * \brief Free a range of hardware pages
 */
void MM_UnmapHWPages(tVAddr VAddr, Uint Number)
{
	Log_KernelPanic("MM", "TODO: Implement MM_UnmapHWPages");
}

// --- Tempory Mappings ---
tVAddr MM_MapTemp(tPAddr PAddr)
{
	Log_KernelPanic("MM", "TODO: Implement MM_MapTemp");
	return 0;
}

void MM_FreeTemp(tVAddr VAddr)
{
	Log_KernelPanic("MM", "TODO: Implement MM_FreeTemp");
	return ;
}


// --- Address Space Clone --
tPAddr MM_Clone(void)
{
	tPAddr	ret;
	
	// #1 Create a copy of the PML4
	ret = MM_AllocPhys();
	if(!ret)	return 0;
	
	Log_KernelPanic("MM", "TODO: Implement MM_Clone");
	
	// #2 Alter the fractal pointer
	// #3 Set Copy-On-Write to all user pages
	// #4 Return
	return 0;
}

void MM_ClearUser(void)
{
	tVAddr	addr = 0;
	// #1 Traverse the structure < 2^47, Deref'ing all pages
	// #2 Free tables/dirs/pdps once they have been cleared
	
	for( addr = 0; addr < 0x800000000000; )
	{
		if( PAGEMAPLVL4(addr >> PML4_SHIFT) & 1 )
		{
			if( PAGEDIRPTR(addr >> PDP_SHIFT) & 1 )
			{
				if( PAGEDIR(addr >> PDIR_SHIFT) & 1 )
				{
					// Page
					if( PAGETABLE(addr >> PTAB_SHIFT) & 1 ) {
						MM_DerefPhys( PAGETABLE(addr >> PTAB_SHIFT) & PADDR_MASK );
						PAGETABLE(addr >> PTAB_SHIFT) = 0;
					}
					addr += 1 << PTAB_SHIFT;
					// Dereference the PDIR Entry
					if( (addr + (1 << PTAB_SHIFT)) >> PDIR_SHIFT != (addr >> PDIR_SHIFT) ) {
						MM_DerefPhys( PAGEMAPLVL4(addr >> PDIR_SHIFT) & PADDR_MASK );
						PAGEDIR(addr >> PDIR_SHIFT) = 0;
					}
				}
				else {
					addr += 1 << PDIR_SHIFT;
					continue;
				}
				// Dereference the PDP Entry
				if( (addr + (1 << PDIR_SHIFT)) >> PDP_SHIFT != (addr >> PDP_SHIFT) ) {
					MM_DerefPhys( PAGEMAPLVL4(addr >> PDP_SHIFT) & PADDR_MASK );
					PAGEDIRPTR(addr >> PDP_SHIFT) = 0;
				}
			}
			else {
				addr += 1 << PDP_SHIFT;
				continue;
			}
			// Dereference the PML4 Entry
			if( (addr + (1 << PDP_SHIFT)) >> PML4_SHIFT != (addr >> PML4_SHIFT) ) {
				MM_DerefPhys( PAGEMAPLVL4(addr >> PML4_SHIFT) & PADDR_MASK );
				PAGEMAPLVL4(addr >> PML4_SHIFT) = 0;
			}
		}
		else {
			addr += (tVAddr)1 << PML4_SHIFT;
			continue;
		}
	}
}

tVAddr MM_NewWorkerStack(void)
{
	Log_KernelPanic("MM", "TODO: Implement MM_NewWorkerStack");
	return 0;
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
