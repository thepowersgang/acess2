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
#include <hal_proc.h>

// === DEBUG OPTIONS ===
#define TRACE_COW	0

// === CONSTANTS ===
#define PHYS_BITS	52	// TODO: Move out
#define VIRT_BITS	48

#define PML4_SHIFT	39
#define PDP_SHIFT	30
#define PDIR_SHIFT	21
#define PTAB_SHIFT	12

#define	PADDR_MASK	0x7FFFFFFF##FFFFF000
#define PAGE_MASK	((1LL << 36)-1)
#define TABLE_MASK	((1LL << 27)-1)
#define PDP_MASK	((1LL << 18)-1)
#define PML4_MASK	((1LL << 9)-1)

#define	PF_PRESENT	0x001
#define	PF_WRITE	0x002
#define	PF_USER		0x004
#define	PF_LARGE	0x080
#define	PF_GLOBAL	0x100
#define	PF_COW		0x200
#define	PF_PAGED	0x400
#define	PF_NX		0x80000000##00000000

// === MACROS ===
#define PAGETABLE(idx)	(*((Uint64*)MM_FRACTAL_BASE+((idx)&PAGE_MASK)))
#define PAGEDIR(idx)	PAGETABLE((MM_FRACTAL_BASE>>12)+((idx)&TABLE_MASK))
#define PAGEDIRPTR(idx)	PAGEDIR((MM_FRACTAL_BASE>>21)+((idx)&PDP_MASK))
#define PAGEMAPLVL4(idx)	PAGEDIRPTR((MM_FRACTAL_BASE>>30)+((idx)&PML4_MASK))

#define TMPCR3()	PAGEMAPLVL4(MM_TMPFRAC_BASE>>39)
#define TMPTABLE(idx)	(*((Uint64*)MM_TMPFRAC_BASE+((idx)&PAGE_MASK)))
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
extern void	Threads_SegFault(tVAddr Addr);
extern char	_UsertextBase[];

// === PROTOTYPES ===
void	MM_InitVirt(void);
//void	MM_FinishVirtualInit(void);
void	MM_int_ClonePageEnt( Uint64 *Ent, void *NextLevel, tVAddr Addr, int bTable );
 int	MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs);
void	MM_int_DumpTablesEnt(tVAddr RangeStart, size_t Length, tPAddr Expected);
//void	MM_DumpTables(tVAddr Start, tVAddr End);
 int	MM_GetPageEntryPtr(tVAddr Addr, BOOL bTemp, BOOL bAllocate, BOOL bLargePage, tPAddr **Pointer);
 int	MM_MapEx(tVAddr VAddr, tPAddr PAddr, BOOL bTemp, BOOL bLarge);
// int	MM_Map(tVAddr VAddr, tPAddr PAddr);
void	MM_Unmap(tVAddr VAddr);
void	MM_int_ClearTableLevel(tVAddr VAddr, int LevelBits, int MaxEnts);
//void	MM_ClearUser(void);
 int	MM_GetPageEntry(tVAddr Addr, tPAddr *Phys, Uint *Flags);

// === GLOBALS ===
tMutex	glMM_TempFractalLock;
tPAddr	gMM_ZeroPage;

// === CODE ===
void MM_InitVirt(void)
{
//	Log_Debug("MMVirt", "&PAGEMAPLVL4(0) = %p", &PAGEMAPLVL4(0));
//	MM_DumpTables(0, -1L);
}

void MM_FinishVirtualInit(void)
{
	PAGEMAPLVL4(0) = 0;
}

/**
 * \brief Clone a page from an entry
 * \param Ent	Pointer to the entry in the PML4/PDP/PD/PT
 * \param NextLevel	Pointer to contents of the entry
 * \param Addr	Dest address
 * \note Used in COW
 */
void MM_int_ClonePageEnt( Uint64 *Ent, void *NextLevel, tVAddr Addr, int bTable )
{
	tPAddr	curpage = *Ent & PADDR_MASK; 
	 int	bCopied = 0;
	
	if( MM_GetRefCount( curpage ) <= 0 ) {
		Log_KernelPanic("MMVirt", "Page %P still marked COW, but unreferenced", curpage);
	}
	if( MM_GetRefCount( curpage ) == 1 )
	{
		*Ent &= ~PF_COW;
		*Ent |= PF_PRESENT|PF_WRITE;
		#if TRACE_COW
		Log_Debug("MMVirt", "COW ent at %p (%p) only %P", Ent, NextLevel, curpage);
		#endif
	}
	else
	{
		void	*tmp;
		tPAddr	paddr;
		
		if( !(paddr = MM_AllocPhys()) ) {
			Threads_SegFault(Addr);
			return ;
		}

		ASSERT(paddr != curpage);
			
		tmp = MM_MapTemp(paddr);
		memcpy( tmp, NextLevel, 0x1000 );
		MM_FreeTemp( tmp );
		
		#if TRACE_COW
		Log_Debug("MMVirt", "COW ent at %p (%p) from %P to %P", Ent, NextLevel, curpage, paddr);
		#endif

		MM_DerefPhys( curpage );
		*Ent &= PF_USER;
		*Ent |= paddr|PF_PRESENT|PF_WRITE;
		
		bCopied = 1;
	}
	INVLPG( (tVAddr)NextLevel );
	
	// Mark COW on contents if it's a PDPT, Dir or Table
	if(bTable) 
	{
		Uint64	*dp = NextLevel;
		 int	i;
		for( i = 0; i < 512; i ++ )
		{
			if( !(dp[i] & PF_PRESENT) )
				continue;
			
			if( bCopied )
				MM_RefPhys( dp[i] & PADDR_MASK );
			if( dp[i] & PF_WRITE ) {
				dp[i] &= ~PF_WRITE;
				dp[i] |= PF_COW;
			}
		}
	}
}

/*
 * \brief Called on a page fault
 */
int MM_PageFault(tVAddr Addr, Uint ErrorCode, tRegs *Regs)
{
//	Log_Debug("MMVirt", "Addr = %p, ErrorCode = %x", Addr, ErrorCode);

	// Catch reserved bits first
	if( ErrorCode & 0x8 )
	{
		Log_Warning("MMVirt", "Reserved bits trashed!");
		Log_Warning("MMVirt", "PML4 Ent   = %P", PAGEMAPLVL4(Addr>>39));
		if( !(PAGEMAPLVL4(Addr>>39) & PF_PRESENT) )	goto print_done;
		Log_Warning("MMVirt", "PDP Ent    = %P", PAGEDIRPTR(Addr>>30));
		if( !(PAGEDIRPTR(Addr>>30) & PF_PRESENT) )	goto print_done;
		Log_Warning("MMVirt", "PDir Ent   = %P", PAGEDIR(Addr>>21));
		if( !(PAGEDIR(Addr>>21) & PF_PRESENT) )	goto print_done;
		Log_Warning("MMVirt", "PTable Ent = %P", PAGETABLE(Addr>>12));
		if( !(PAGETABLE(Addr>>12) & PF_PRESENT) )	goto print_done;
	print_done:
		
		for(;;);
	}

	// TODO: Implement Copy-on-Write
	#if 1
	if( PAGEMAPLVL4(Addr>>39) & PF_PRESENT
	 && PAGEDIRPTR (Addr>>30) & PF_PRESENT
	 && PAGEDIR    (Addr>>21) & PF_PRESENT
	 && PAGETABLE  (Addr>>12) & PF_PRESENT )
	{
		// PML4 Entry
		if( PAGEMAPLVL4(Addr>>39) & PF_COW )
		{
			tPAddr	*dp = &PAGEDIRPTR((Addr>>39)*512);
			MM_int_ClonePageEnt( &PAGEMAPLVL4(Addr>>39), dp, Addr, 1 );
//			MM_DumpTables(Addr>>39 << 39, (((Addr>>39) + 1) << 39) - 1);
		}
		// PDP Entry
		if( PAGEDIRPTR(Addr>>30) & PF_COW )
		{
			tPAddr	*dp = &PAGEDIR( (Addr>>30)*512 );
			MM_int_ClonePageEnt( &PAGEDIRPTR(Addr>>30), dp, Addr, 1 );
//			MM_DumpTables(Addr>>30 << 30, (((Addr>>30) + 1) << 30) - 1);
		}
		// PD Entry
		if( PAGEDIR(Addr>>21) & PF_COW )
		{
			tPAddr	*dp = &PAGETABLE( (Addr>>21)*512 );
			MM_int_ClonePageEnt( &PAGEDIR(Addr>>21), dp, Addr, 1 );
//			MM_DumpTables(Addr>>21 << 21, (((Addr>>21) + 1) << 21) - 1);
		}
		// PT Entry
		if( PAGETABLE(Addr>>12) & PF_COW )
		{
			MM_int_ClonePageEnt( &PAGETABLE(Addr>>12), (void*)(Addr & ~0xFFF), Addr, 0 );
			INVLPG( Addr & ~0xFFF );
			return 0;
		}
	}
	#endif
	
	// If it was a user, tell the thread handler
	if(ErrorCode & 4) {
		Warning("User %s %s memory%s",
			(ErrorCode&2?"write to":"read from"),
			(ErrorCode&1?"bad/locked":"non-present"),
			(ErrorCode&16?" (Instruction Fetch)":"")
			);
		Warning("User Pagefault: Instruction at %04x:%p accessed %p",
			Regs->CS, Regs->RIP, Addr);
		__asm__ __volatile__ ("sti");	// Restart IRQs
		Error_Backtrace(Regs->RIP, Regs->RBP);
		Threads_SegFault(Addr);
		return 0;
	}
	
	// Kernel #PF
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
	
	Log("Thread %i - Code at %p accessed %p", Threads_GetTID(), Regs->RIP, Addr);
	// Print Stack Backtrace
	Error_Backtrace(Regs->RIP, Regs->RBP);
	
	MM_DumpTables(0, -1);

	return 1;	
}

void MM_int_DumpTablesEnt(tVAddr RangeStart, size_t Length, tPAddr Expected)
{
	#define CANOICAL(addr)	((addr)&0x800000000000?(addr)|0xFFFF000000000000:(addr))
	LogF("%016llx => ", CANOICAL(RangeStart));
//	LogF("%6llx %6llx %6llx %016llx => ",
//		MM_GetPhysAddr( (tVAddr)&PAGEDIRPTR(RangeStart>>30) ),
//		MM_GetPhysAddr( (tVAddr)&PAGEDIR(RangeStart>>21) ),
//		MM_GetPhysAddr( (tVAddr)&PAGETABLE(RangeStart>>12) ),
//		CANOICAL(RangeStart)
//		);
	if( gMM_ZeroPage && (PAGETABLE(RangeStart>>12) & PADDR_MASK) == gMM_ZeroPage )
		LogF("%13s", "zero" );
	else
		LogF("%13llx", PAGETABLE(RangeStart>>12) & PADDR_MASK );
	LogF(" : 0x%6llx (%c%c%c%c%c%c)\r\n",
		Length,
		(Expected & PF_GLOBAL ? 'G' : '-'),
		(Expected & PF_NX ? '-' : 'x'),
		(Expected & PF_PAGED ? 'p' : '-'),
		(Expected & PF_COW ? 'C' : '-'),
		(Expected & PF_USER ? 'U' : '-'),
		(Expected & PF_WRITE ? 'W' : '-')
		);
	#undef CANOICAL
}

/**
 * \brief Dumps the layout of the page tables
 */
void MM_DumpTables(tVAddr Start, tVAddr End)
{
	const tPAddr	FIXED_BITS = PF_PRESENT|PF_WRITE|PF_USER|PF_COW|PF_PAGED|PF_NX|PF_GLOBAL;
	const tPAddr	CHANGEABLE_BITS = ~FIXED_BITS & 0xFFF;
	const tPAddr	MASK = ~CHANGEABLE_BITS;	// Physical address and access bits
	tVAddr	rangeStart = 0;
	tPAddr	expected = CHANGEABLE_BITS;	// CHANGEABLE_BITS is used because it's not a vaild value
	tVAddr	curPos;
	Uint	page;
	tPAddr	expected_pml4 = PF_WRITE|PF_USER;	
	tPAddr	expected_pdp = PF_WRITE|PF_USER;	
	tPAddr	expected_pd = PF_WRITE|PF_USER;	

	Log("Table Entries: (%p to %p)", Start, End);
	
	End &= (1L << 48) - 1;
	
	Start >>= 12;	End >>= 12;
	
	for(page = Start, curPos = Start<<12;
		page < End;
		curPos += 0x1000, page++)
	{
		//Debug("&PAGEMAPLVL4(%i page>>27) = %p", page>>27, &PAGEMAPLVL4(page>>27));
		//Debug("&PAGEDIRPTR(%i page>>18) = %p", page>>18, &PAGEDIRPTR(page>>18));
		//Debug("&PAGEDIR(%i page>>9) = %p", page>>9, &PAGEDIR(page>>9));
		//Debug("&PAGETABLE(%i page) = %p", page, &PAGETABLE(page));
		
		// End of a range
		if(!(PAGEMAPLVL4(page>>27) & PF_PRESENT)
		||  (PAGEMAPLVL4(page>>27) & FIXED_BITS) != expected_pml4
		|| !(PAGEDIRPTR(page>>18) & PF_PRESENT)
		||  (PAGEDIRPTR(page>>18) & FIXED_BITS) != expected_pdp
		|| !(PAGEDIR(page>>9) & PF_PRESENT)
		||  (PAGEDIR(page>>9) & FIXED_BITS) != expected_pd
		|| !(PAGETABLE(page) & PF_PRESENT)
		||  (PAGETABLE(page) & MASK) != expected)
		{			
			if(expected != CHANGEABLE_BITS)
			{
				// Merge
				expected &= expected_pml4 | ~(PF_WRITE|PF_USER);
				expected &= expected_pdp  | ~(PF_WRITE|PF_USER);
				expected &= expected_pd   | ~(PF_WRITE|PF_USER);
				expected |= expected_pml4 & PF_NX;
				expected |= expected_pdp  & PF_NX;
				expected |= expected_pd   & PF_NX;
				Log("expected (pml4 = %x, pdp = %x, pd = %x)",
					expected_pml4, expected_pdp, expected_pd);
				// Dump
				MM_int_DumpTablesEnt( rangeStart, curPos - rangeStart, expected );
				expected = CHANGEABLE_BITS;
			}
			
			if( curPos == 0x800000000000L )
				curPos = 0xFFFF800000000000L;
		
			if( !(PAGEMAPLVL4(page>>27) & PF_PRESENT) ) {
				page += (1 << 27) - 1;
				curPos += (1L << 39) - 0x1000;
				continue;
			}
			if( !(PAGEDIRPTR(page>>18) & PF_PRESENT) ) {
				page += (1 << 18) - 1;
				curPos += (1L << 30) - 0x1000;
				continue;
			}
			if( !(PAGEDIR(page>>9) & PF_PRESENT) ) {
				page += (1 << 9) - 1;
				curPos += (1L << 21) - 0x1000;
				continue;
			}
			if( !(PAGETABLE(page) & PF_PRESENT) )	continue;
			
			expected = (PAGETABLE(page) & MASK);
			expected_pml4 = (PAGEMAPLVL4(page>>27) & FIXED_BITS);
			expected_pdp  = (PAGEDIRPTR (page>>18) & FIXED_BITS);
			expected_pd   = (PAGEDIR    (page>> 9) & FIXED_BITS);
			rangeStart = curPos;
		}
		if(gMM_ZeroPage && (expected & PADDR_MASK) == gMM_ZeroPage )
			expected = expected;
		else if(expected != CHANGEABLE_BITS)
			expected += 0x1000;
	}
	
	if(expected != CHANGEABLE_BITS) {
		// Merge
		
		// Dump
		MM_int_DumpTablesEnt( rangeStart, curPos - rangeStart, expected );
		expected = 0;
	}
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
	 int	i, size;
	
	#define BITMASK(bits)	( (1LL << (bits))-1 )

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
		pmlevels[2] = &pmlevels[3][(MM_FRACTAL_BASE>>12)&BITMASK(VIRT_BITS-12)];	// PDIR
		pmlevels[1] = &pmlevels[2][(MM_FRACTAL_BASE>>21)&BITMASK(VIRT_BITS-21)];	// PDPT
		pmlevels[0] = &pmlevels[1][(MM_FRACTAL_BASE>>30)&BITMASK(VIRT_BITS-30)];	// PML4
	}
	
	// Mask address
	Addr &= (1ULL << 48)-1;
	
	for( size = 39, i = 0; size > 12; size -= 9, i ++ )
	{
		Uint64	*ent = &pmlevels[i][Addr >> size];
//		INVLPG( &pmlevels[i][ (Addr >> ADDR_SIZES[i]) & 
		
		// Check for a free large page slot
		// TODO: Better support with selectable levels
		if( (Addr & ((1ULL << size)-1)) == 0 && bLargePage )
		{
			if(Pointer)	*Pointer = ent;
			return size;
		}
		// Allocate an entry if required
		if( !(*ent & PF_PRESENT) )
		{
			if( !bAllocate )	return -4;	// If allocation is not requested, error
			if( !(tmp = MM_AllocPhys()) )	return -2;
			*ent = tmp | 3;
			if( Addr < 0x800000000000 )
				*ent |= PF_USER;
			INVLPG( &pmlevels[i+1][ (Addr>>size)*512 ] );
			memset( &pmlevels[i+1][ (Addr>>size)*512 ], 0, 0x1000 );
			LOG("Init PML%i ent 0x%x %p with %P (*ent = %P)", 4 - i,
				Addr>>size, (Addr>>size) << size, tmp, *ent);
		}
		// Catch large pages
		else if( *ent & PF_LARGE )
		{
			// Alignment
			if( (Addr & ((1ULL << size)-1)) != 0 )	return -3;
			if(Pointer)	*Pointer = ent;
			return size;	// Large page warning
		}
	}
	
	// And, set the page table entry
	if(Pointer)	*Pointer = &pmlevels[i][Addr >> size];
	return size;
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
	
	ENTER("pVAddr PPAddr", VAddr, PAddr);
	
	// Get page pointer (Allow allocating)
	rv = MM_GetPageEntryPtr(VAddr, bTemp, 1, bLarge, &ent);
	if(rv < 0)	LEAVE_RET('i', 0);
	
	if( *ent & 1 )	LEAVE_RET('i', 0);
	
	*ent = PAddr | 3;

	if( VAddr < 0x800000000000 )
		*ent |= PF_USER;

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

tPAddr MM_AllocateZero(tVAddr VAddr)
{
	tPAddr	ret = gMM_ZeroPage;
	
	MM_GetPageEntryPtr(VAddr, 0, 1, 0, NULL);

	if(!gMM_ZeroPage) {
		ret = gMM_ZeroPage = MM_AllocPhys();
		MM_RefPhys(ret);	// Don't free this please
		MM_Map(VAddr, ret);
		memset((void*)VAddr, 0, 0x1000);
	}
	else {
		MM_Map(VAddr, ret);
	}
	MM_RefPhys(ret);	// Refernce for this map
	MM_SetFlags(VAddr, MM_PFLAG_COW, MM_PFLAG_COW);
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
	
	if( !(*ptr & 1) )	return 0;
	
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
	INVLPG_ALL();
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

/**
 * \brief Check if the provided buffer is valid
 * \return Boolean valid
 */
int MM_IsValidBuffer(tVAddr Addr, size_t Size)
{
	 int	bIsUser;
	Uint64	pml4, pdp, dir, tab;

	Size += Addr & (PAGE_SIZE-1);
	Addr &= ~(PAGE_SIZE-1);
	Addr &= ((1UL << 48)-1);	// Clap to address space

	pml4 = Addr >> 39;
	pdp = Addr >> 30;
	dir = Addr >> 21;
	tab = Addr >> 12;

	if( !(PAGEMAPLVL4(pml4) & 1) )	return 0;
	if( !(PAGEDIRPTR(pdp) & 1) )	return 0;
	if( !(PAGEDIR(dir) & 1) )	return 0;
	if( !(PAGETABLE(tab) & 1) )	return 0;
	
	bIsUser = !!(PAGETABLE(tab) & PF_USER);

	while( Size >= PAGE_SIZE )
	{
		if( (tab & 511) == 0 )
		{
			dir ++;
			if( ((dir >> 9) & 511) == 0 )
			{
				pdp ++;
				if( ((pdp >> 18) & 511) == 0 )
				{
					pml4 ++;
					if( !(PAGEMAPLVL4(pml4) & 1) )	return 0;
				}
				if( !(PAGEDIRPTR(pdp) & 1) )	return 0;
			}
			if( !(PAGEDIR(dir) & 1) )	return 0;
		}
		
		if( !(PAGETABLE(tab) & 1) )   return 0;
		if( bIsUser && !(PAGETABLE(tab) & PF_USER) )	return 0;

		tab ++;
		Size -= PAGE_SIZE;
	}
	return 1;
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
		
//		Log_Debug("MMVirt", "Mapping %i pages to %p (base %P)", Number, ret-Number*0x1000, PAddr);

		PAddr += 0x1000 * Number;
		
		while( Number -- )
		{
			ret -= 0x1000;
			PAddr -= 0x1000;
			MM_Map(ret, PAddr);
			MM_RefPhys(PAddr);
		}
		
		return ret;
	}
	
	Log_Error("MM", "MM_MapHWPages - No space for %i pages", Number);
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
		MM_DerefPhys( MM_GetPhysAddr(VAddr) );
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
		MM_DerefPhys(phys);
		return ret;
	}
	
	// Slow Allocate
	phys = MM_AllocPhysRange(Pages, MaxBits);
	// - Was it allocated?
	if(phys == 0)	return 0;
	
	// Allocated successfully, now map
	ret = MM_MapHWPages(phys, Pages);
	// MapHWPages references the pages, so deref them back down to 1
	for(;Pages--;phys+=0x1000)
		MM_DerefPhys(phys);
	if( ret == 0 ) {
		// If it didn't map, free then return 0
		return 0;
	}
	
	*PhysAddr = phys;
	return ret;
}

// --- Tempory Mappings ---
void *MM_MapTemp(tPAddr PAddr)
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
		MM_RefPhys(PAddr);
		INVLPG(ret);
		return (void*)ret;
	}
	return 0;
}

void MM_FreeTemp(void *Ptr)
{
	MM_Deallocate((tVAddr)Ptr);
	return ;
}


// --- Address Space Clone --
tPAddr MM_Clone(void)
{
	tPAddr	ret;
	 int	i;
	tVAddr	kstackbase;

	// #1 Create a copy of the PML4
	ret = MM_AllocPhys();
	if(!ret)	return 0;
	
	// #2 Alter the fractal pointer
	Mutex_Acquire(&glMM_TempFractalLock);
	TMPCR3() = ret | 3;
	INVLPG_ALL();
	
	// #3 Set Copy-On-Write to all user pages
	if( Threads_GetPID() != 0 )
	{
		for( i = 0; i < 256; i ++)
		{
			if( PAGEMAPLVL4(i) & PF_WRITE ) {
				PAGEMAPLVL4(i) |= PF_COW;
				PAGEMAPLVL4(i) &= ~PF_WRITE;
			}
	
			TMPMAPLVL4(i) = PAGEMAPLVL4(i);
//			Log_Debug("MM", "TMPMAPLVL4(%i) = 0x%016llx", i, TMPMAPLVL4(i));
			if( !(TMPMAPLVL4(i) & PF_PRESENT) )	continue ;
			
			MM_RefPhys( TMPMAPLVL4(i) & PADDR_MASK );
		}
	}
	else
	{
		for( i = 0; i < 256; i ++ )
		{
			TMPMAPLVL4(i) = 0;
		}
	}
	
	// #4 Map in kernel pages
	for( i = 256; i < 512; i ++ )
	{
		// Skip addresses:
		// 320 0xFFFFA....	- Kernel Stacks
		if( i == MM_KSTACK_BASE>>39 )	continue;
		// 509 0xFFFFFE0..	- Fractal mapping
		if( i == MM_FRACTAL_BASE>>39 )	continue;
		// 510 0xFFFFFE8..	- Temp fractal mapping
		if( i == MM_TMPFRAC_BASE>>39 )	continue;
		
		TMPMAPLVL4(i) = PAGEMAPLVL4(i);
		if( TMPMAPLVL4(i) & 1 )
			MM_RefPhys( TMPMAPLVL4(i) & PADDR_MASK );
	}

	// Mark Per-Process data as COW
	TMPMAPLVL4(MM_PPD_BASE>>39) |= PF_COW;
	TMPMAPLVL4(MM_PPD_BASE>>39) &= ~PF_WRITE;
	
	// #5 Set fractal mapping
	TMPMAPLVL4(MM_FRACTAL_BASE>>39) = ret | 3;	// Main
	TMPMAPLVL4(MM_TMPFRAC_BASE>>39) = 0;	// Temp
	
	// #6 Create kernel stack
	//  tThread->KernelStack is the top
	//  There is 1 guard page below the stack
	kstackbase = Proc_GetCurThread()->KernelStack - KERNEL_STACK_SIZE;

	// Clone stack
	TMPMAPLVL4(MM_KSTACK_BASE >> PML4_SHIFT) = 0;
	for( i = 1; i < KERNEL_STACK_SIZE/0x1000; i ++ )
	{
		tPAddr	phys = MM_AllocPhys();
		void	*tmpmapping;
		MM_MapEx(kstackbase+i*0x1000, phys, 1, 0);
		
		tmpmapping = MM_MapTemp(phys);
		if( MM_GetPhysAddr( kstackbase+i*0x1000 ) )
			memcpy(tmpmapping, (void*)(kstackbase+i*0x1000), 0x1000);
		else
			memset(tmpmapping, 0, 0x1000);
//		if( i == 0xF )
//			Debug_HexDump("MM_Clone: *tmpmapping = ", (void*)tmpmapping, 0x1000);
		MM_FreeTemp(tmpmapping);
	}
	
//	MAGIC_BREAK();

	// #7 Return
	TMPCR3() = 0;
	INVLPG_ALL();
	Mutex_Release(&glMM_TempFractalLock);
//	Log("MM_Clone: RETURN %P", ret);
	return ret;
}

void MM_int_ClearTableLevel(tVAddr VAddr, int LevelBits, int MaxEnts)
{
	Uint64	* const table_bases[] = {&PAGETABLE(0), &PAGEDIR(0), &PAGEDIRPTR(0), &PAGEMAPLVL4(0)};
	Uint64	*table = table_bases[(LevelBits-12)/9] + (VAddr >> LevelBits);
	 int	i;
//	Log("MM_int_ClearTableLevel: (VAddr=%p, LevelBits=%i, MaxEnts=%i)", VAddr, LevelBits, MaxEnts);
	for( i = 0; i < MaxEnts; i ++ )
	{
		// Skip non-present tables
		if( !(table[i] & PF_PRESENT) ) {
			table[i] = 0;
			continue ;
		}
	
		if( (table[i] & PF_COW) && MM_GetRefCount(table[i] & PADDR_MASK) > 1 ) {
			MM_DerefPhys(table[i] & PADDR_MASK);
			table[i] = 0;
			continue ;
		}
		// Clear table contents (if it is a table)
		if( LevelBits > 12 )
			MM_int_ClearTableLevel(VAddr + ((tVAddr)i << LevelBits), LevelBits-9, 512);
		MM_DerefPhys(table[i] & PADDR_MASK);
		table[i] = 0;
	}
}

void MM_ClearUser(void)
{
	MM_int_ClearTableLevel(0, 39, 256);	
}

tVAddr MM_NewWorkerStack(void *StackData, size_t StackSize)
{
	tVAddr	ret;
	tPAddr	phys;
	 int	i;
	
	// #1 Set temp fractal to PID0
	Mutex_Acquire(&glMM_TempFractalLock);
	TMPCR3() = ((tPAddr)gInitialPML4 - KERNEL_BASE) | 3;
	INVLPG_ALL();
	
	// #2 Scan for a free stack addresss < 2^47
	for(ret = 0x100000; ret < (1ULL << 47); ret += KERNEL_STACK_SIZE)
	{
		tPAddr	*ptr;
		if( MM_GetPageEntryPtr(ret, 1, 0, 0, &ptr) <= 0 )	break;
		if( !(*ptr & 1) )	break;
	}
	if( ret >= (1ULL << 47) ) {
		Mutex_Release(&glMM_TempFractalLock);
		return 0;
	}
	
	// #3 Map all save the last page in the range
	//  - This acts as as guard page
	MM_GetPageEntryPtr(ret, 1, 1, 0, NULL);	// Make sure tree is allocated
	for( i = 0; i < KERNEL_STACK_SIZE/0x1000 - 1; i ++ )
	{
		phys = MM_AllocPhys();
		if(!phys) {
			// TODO: Clean up
			Log_Error("MM", "MM_NewWorkerStack - Unable to allocate page");
			return 0;
		}
		MM_MapEx(ret + i*0x1000, phys, 1, 0);
		MM_SetFlags(ret + i*0x1000, MM_PFLAG_KERNEL|MM_PFLAG_RO, MM_PFLAG_KERNEL);
	}

	// Copy data
	if( StackSize > 0x1000 ) {
		Log_Error("MM", "MM_NewWorkerStack: StackSize(0x%x) > 0x1000, cbf handling", StackSize);
	}
	else {
		void	*tmp_addr, *dest;
		tmp_addr = MM_MapTemp(phys);
		dest = (char*)tmp_addr + (0x1000 - StackSize);
		memcpy( dest, StackData, StackSize );
		Log_Debug("MM", "MM_NewWorkerStack: %p->%p %i bytes (i=%i)", StackData, dest, StackSize, i);
		Log_Debug("MM", "MM_NewWorkerStack: ret = %p", ret);
		MM_FreeTemp(tmp_addr);
	}

	TMPCR3() = 0;
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
		if(MM_GetPhysAddr(base+KERNEL_STACK_SIZE-0x1000) != 0)
			continue;
		
		//Log("MM_NewKStack: Found one at %p", base + KERNEL_STACK_SIZE);
		for( i = 0x1000; i < KERNEL_STACK_SIZE; i += 0x1000)
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
