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
#include <common.h>
#include <mm_phys.h>
#include <proc.h>

#define KERNEL_STACKS	0xF0000000
#define	KERNEL_STACK_SIZE	0x00002000
#define KERNEL_STACK_END	0xFD000000
#define PAGE_TABLE_ADDR	0xFD000000
#define PAGE_DIR_ADDR	0xFD3F4000
#define PAGE_CR3_ADDR	0xFD3F4FD0
#define TMP_CR3_ADDR	0xFD3F4FD4	// Part of core instead of temp
#define TMP_DIR_ADDR	0xFD3F5000	// Same
#define TMP_TABLE_ADDR	0xFD400000
#define	HW_MAP_ADDR		0xFD800000
#define	HW_MAP_MAX		0xFEFF0000
#define	NUM_HW_PAGES	((HW_MAP_MAX-HW_MAP_ADDR)/0x1000)
#define	TEMP_MAP_ADDR	0xFEFF0000	// Allows 16 "temp" pages
#define	NUM_TEMP_PAGES	16

#define	PF_PRESENT	0x1
#define	PF_WRITE	0x2
#define	PF_USER		0x4
#define	PF_COW		0x200
#define	PF_PAGED	0x400

#define INVLPG(addr)	__asm__ __volatile__ ("invlpg (%0)"::"r"(addr))

// === IMPORTS ===
extern Uint32	gaInitPageDir[1024];
extern Uint32	gaInitPageTable[1024];

// === PROTOTYPES ===
void	MM_PreinitVirtual();
void	MM_InstallVirtual();
void	MM_PageFault(Uint Addr, Uint ErrorCode, tRegs *Regs);
void	MM_DumpTables(tVAddr Start, tVAddr End);
tPAddr	MM_DuplicatePage(Uint VAddr);

// === GLOBALS ===
tPAddr	*gaPageTable = (void*)PAGE_TABLE_ADDR;
tPAddr	*gaPageDir = (void*)PAGE_DIR_ADDR;
tPAddr	*gaPageCR3 = (void*)PAGE_CR3_ADDR;
tPAddr	*gaTmpTable = (void*)TMP_TABLE_ADDR;
tPAddr	*gaTmpDir = (void*)TMP_DIR_ADDR;
tPAddr	*gTmpCR3 = (void*)TMP_CR3_ADDR;
 int	gilTempMappings = 0;

// === CODE ===
/**
 * \fn void MM_PreinitVirtual()
 * \brief Maps the fractal mappings
 */
void MM_PreinitVirtual()
{
	gaInitPageDir[ 0 ] = 0;
	gaInitPageDir[ PAGE_TABLE_ADDR >> 22 ] = ((Uint)&gaInitPageDir - KERNEL_BASE) | 3;
}

/**
 * \fn void MM_InstallVirtual()
 * \brief Sets up the constant page mappings
 */
void MM_InstallVirtual()
{
	 int	i;
	
	// --- Pre-Allocate kernel tables
	for( i = KERNEL_BASE>>22; i < 1024; i ++ )
	{
		if( gaPageDir[ i ] )	continue;
		// Skip stack tables, they are process unique
		if( i > KERNEL_STACKS >> 22 && i < KERNEL_STACK_END >> 22) {
			gaPageDir[ i ] = 0;
			continue;
		}
		// Preallocate table
		gaPageDir[ i ] = MM_AllocPhys() | 3;
		INVLPG( &gaPageTable[i*1024] );
		memset( &gaPageTable[i*1024], 0, 0x1000 );
	}
}

/**
 * \fn void MM_PageFault(Uint Addr, Uint ErrorCode, tRegs *Regs)
 * \brief Called on a page fault
 */
void MM_PageFault(Uint Addr, Uint ErrorCode, tRegs *Regs)
{
	//ENTER("xAddr bErrorCode", Addr, ErrorCode);
	
	// -- Check for COW --
	if( gaPageDir  [Addr>>22] & PF_PRESENT
	 && gaPageTable[Addr>>12] & PF_PRESENT
	 && gaPageTable[Addr>>12] & PF_COW )
	{
		tPAddr	paddr;
		if(MM_GetRefCount( gaPageTable[Addr>>12] & ~0xFFF ) == 0)
		{
			gaPageTable[Addr>>12] &= ~PF_COW;
			gaPageTable[Addr>>12] |= PF_PRESENT|PF_WRITE;
		}
		else
		{
			paddr = MM_DuplicatePage( Addr );
			MM_DerefPhys( gaPageTable[Addr>>12] & ~0xFFF );
			gaPageTable[Addr>>12] &= PF_USER;
			gaPageTable[Addr>>12] |= paddr|PF_PRESENT|PF_WRITE;
		}
		INVLPG( Addr & ~0xFFF );
		//LEAVE('-')
		return;
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
	
	Log("gaPageDir[0x%x] = 0x%x", Addr>>22, gaPageDir[Addr>>22]);
	if( gaPageDir[Addr>>22] & PF_PRESENT )
		Log("gaPageTable[0x%x] = 0x%x", Addr>>12, gaPageTable[Addr>>12]);
	
	MM_DumpTables(0, -1);	
	
	Panic("Page Fault at 0x%x\n", Regs->eip);
}

/**
 * \fn void MM_DumpTables(Uint Start, Uint End)
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
	for(page = Start, curPos = Start<<12;
		page < End;
		curPos += 0x1000, page++)
	{
		if( !(gaPageDir[curPos>>22] & PF_PRESENT)
		||  !(gaPageTable[page] & PF_PRESENT)
		||  (gaPageTable[page] & MASK) != expected)
		{
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
 * \fn tPAddr MM_Allocate(Uint VAddr)
 */
tPAddr MM_Allocate(Uint VAddr)
{
	tPAddr	paddr;
	// Check if the directory is mapped
	if( gaPageDir[ VAddr >> 22 ] == 0 )
	{
		// Allocate directory
		paddr = MM_AllocPhys();
		if( paddr == 0 ) {
			Warning("MM_Allocate - Out of Memory (Called by %p)", __builtin_return_address(0));
			return 0;
		}
		// Map
		gaPageDir[ VAddr >> 22 ] = paddr | 3;
		// Mark as user
		if(VAddr < MM_USER_MAX)	gaPageDir[ VAddr >> 22 ] |= PF_USER;
		
		INVLPG( &gaPageDir[ VAddr >> 22 ] );
		memsetd( &gaPageTable[ (VAddr >> 12) & ~0x3FF ], 0, 1024 );
	}
	// Check if the page is already allocated
	else if( gaPageTable[ VAddr >> 12 ] != 0 ) {
		Warning("MM_Allocate - Allocating to used address (%p)", VAddr);
		return gaPageTable[ VAddr >> 12 ] & ~0xFFF;
	}
	
	// Allocate
	paddr = MM_AllocPhys();
	if( paddr == 0 ) {
		Warning("MM_Allocate - Out of Memory (Called by %p)", __builtin_return_address(0));
		return 0;
	}
	// Map
	gaPageTable[ VAddr >> 12 ] = paddr | 3;
	// Mark as user
	if(VAddr < MM_USER_MAX)	gaPageTable[ VAddr >> 12 ] |= PF_USER;
	// Invalidate Cache for address
	INVLPG( VAddr & ~0xFFF );
	
	return paddr;
}

/**
 * \fn void MM_Deallocate(Uint VAddr)
 */
void MM_Deallocate(Uint VAddr)
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
 * \fn tPAddr MM_GetPhysAddr(Uint Addr)
 * \brief Checks if the passed address is accesable
 */
tPAddr MM_GetPhysAddr(Uint Addr)
{
	if( !(gaPageDir[Addr >> 22] & 1) )
		return 0;
	if( !(gaPageTable[Addr >> 12] & 1) )
		return 0;
	return (gaPageTable[Addr >> 12] & ~0xFFF) | (Addr & 0xFFF);
}

/**
 * \fn void MM_SetCR3(Uint CR3)
 * \brief Sets the current process space
 */
void MM_SetCR3(Uint CR3)
{
	__asm__ __volatile__ ("mov %0, %%cr3"::"r"(CR3));
}

/**
 * \fn int MM_Map(Uint VAddr, tPAddr PAddr)
 * \brief Map a physical page to a virtual one
 */
int MM_Map(Uint VAddr, tPAddr PAddr)
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
 * \fn Uint MM_ClearUser()
 * \brief Clear user's address space
 */
Uint MM_ClearUser()
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
	}
	
	
	return *gTmpCR3;
}

/**
 * \fn Uint MM_Clone()
 * \brief Clone the current address space
 */
Uint MM_Clone()
{
	Uint	i, j;
	Uint	kStackBase = gCurrentThread->KernelStack - KERNEL_STACK_SIZE;
	void	*tmp;
	
	//ENTER("");
	
	// Create Directory Table
	*gTmpCR3 = MM_AllocPhys() | 3;
	INVLPG( gaTmpDir );
	//LOG("Allocated Directory (%x)", *gTmpCR3);
	memsetd( gaTmpDir, 0, 1024 );
	
	// Copy Tables
	for(i=0;i<768;i++)
	{
		// Check if table is allocated
		if( !(gaPageDir[i] & PF_PRESENT) ) {
			gaTmpDir[i] = 0;
			continue;
		}
		
		// Allocate new table
		gaTmpDir[i] = MM_AllocPhys() | (gaPageDir[i] & 7);
		INVLPG( &gaTmpTable[i*1024] );
		// Fill
		for( j = 0; j < 1024; j ++ )
		{
			if( !(gaPageTable[i*1024+j] & PF_PRESENT) ) {
				gaTmpTable[i*1024+j] = 0;
				continue;
			}
			
			// Refrence old page
			MM_RefPhys( gaPageTable[i*1024+j] & ~0xFFF );
			// Add to new table
			if(gaPageTable[i*1024+j] & PF_WRITE) {
				gaTmpTable[i*1024+j] = (gaPageTable[i*1024+j] & ~PF_WRITE) | PF_COW;
				gaPageTable[i*1024+j] = (gaPageTable[i*1024+j] & ~PF_WRITE) | PF_COW;
			}
			else
				gaTmpTable[i*1024+j] = gaPageTable[i*1024+j];
		}
	}
	
	// Map in kernel tables (and make fractal mapping)
	for( i = 768; i < 1024; i ++ )
	{
		// Fractal
		if( i == (PAGE_TABLE_ADDR >> 22) ) {
			gaTmpDir[ PAGE_TABLE_ADDR >> 22 ] = *gTmpCR3;
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
		i < KERNEL_STACK_END >> 22;
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
	
	//LEAVE('x', *gTmpCR3 & ~0xFFF);
	return *gTmpCR3 & ~0xFFF;
}

/**
 * \fn Uint MM_NewKStack()
 * \brief Create a new kernel stack
 */
Uint MM_NewKStack()
{
	Uint	base = KERNEL_STACKS;
	Uint	i;
	for(;base<KERNEL_STACK_END;base+=KERNEL_STACK_SIZE)
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
 * \fn void MM_SetFlags(Uint VAddr, Uint Flags, Uint Mask)
 * \brief Sets the flags on a page
 */
void MM_SetFlags(Uint VAddr, Uint Flags, Uint Mask)
{
	tPAddr	*ent;
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
 * \fn tPAddr MM_DuplicatePage(Uint VAddr)
 * \brief Duplicates a virtual page to a physical one
 */
tPAddr MM_DuplicatePage(Uint VAddr)
{
	tPAddr	ret;
	Uint	temp;
	 int	wasRO = 0;
	
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
	
	return ret;
}

/**
 * \fn Uint MM_MapTemp(tPAddr PAddr)
 * \brief Create a temporary memory mapping
 * \todo Show Luigi Barone (C Lecturer) and see what he thinks
 */
Uint MM_MapTemp(tPAddr PAddr)
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
		Proc_Yield();
	}
}

/**
 * \fn void MM_FreeTemp(Uint PAddr)
 * \brief Free's a temp mapping
 */
void MM_FreeTemp(Uint VAddr)
{
	 int	i = VAddr >> 12;
	//ENTER("xVAddr", VAddr);
	
	if(i >= (TEMP_MAP_ADDR >> 12))
		gaPageTable[ i ] = 0;
	
	//LEAVE('-');
}

/**
 * \fn Uint MM_MapHWPage(tPAddr PAddr, Uint Number)
 * \brief Allocates a contigous number of pages
 */
Uint MM_MapHWPage(tPAddr PAddr, Uint Number)
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
 * \fn void MM_UnmapHWPage(Uint VAddr, Uint Number)
 * \brief Unmap a hardware page
 */
void MM_UnmapHWPage(Uint VAddr, Uint Number)
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
EXPORT(MM_MapHWPage);
EXPORT(MM_UnmapHWPage);
