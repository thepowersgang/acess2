/*
 * Acess2 x86_64 Port
 * 
 * Virtual Memory Manager
 */
#include <acess.h>
#include <mm_virt.h>

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
#define	PF_COW		0x200
#define	PF_PAGED	0x400
#define	PF_NX		0x80000000##00000000

// === MACROS ===
#define PAGETABLE(idx)	(*((tPAddr*)MM_FRACTAL_BASE+((idx)&PAGE_MASK)))
#define PAGEDIR(idx)	PAGETABLE((MM_FRACTAL_BASE>>12)+((idx)&TABLE_MASK))
#define PAGEDIRPTR(idx)	PAGEDIR((MM_FRACTAL_BASE>>21)+((idx)&PDP_MASK))
#define PAGEMAPLVL4(idx)	PAGEDIRPTR((MM_FRACTAL_BASE>>30)+((idx)&PML4_MASK))

// === GLOBALS ===

// === CODE ===
void MM_InitVirt(void)
{
	
}

void MM_FinishVirtualInit(void)
{
	
}

/**
 * \brief Map a physical page to a virtual one
 */
int MM_Map(tVAddr VAddr, tPAddr PAddr)
{
	tPAddr	tmp;
	
	Log("MM_Map: (VAddr=0x%x, PAddr=0x%x)", VAddr, PAddr);
	
	// Check PML4
	Log(" MM_Map: &PAGEMAPLVL4(%x) = %x", VAddr >> 39, &PAGEMAPLVL4(VAddr >> 39));
	Log(" MM_Map: &PAGEDIRPTR(%x) = %x", VAddr >> 30, &PAGEDIRPTR(VAddr >> 30));
	Log(" MM_Map: &PAGEDIR(%x) = %x", VAddr >> 21, &PAGEDIR(VAddr >> 21));
	Log(" MM_Map: &PAGETABLE(%x) = %x", VAddr >> 12, &PAGETABLE(VAddr >> 12));
	Log(" MM_Map: &PAGETABLE(0) = %x", &PAGETABLE(0));
	if( !(PAGEMAPLVL4(VAddr >> 39) & 1) )
	{
		tmp = MM_AllocPhys();
		if(!tmp)	return 0;
		PAGEMAPLVL4(VAddr >> 39) = tmp | 3;
		memset( &PAGEDIRPTR( (VAddr>>39)<<9 ), 0, 4096 );
	}
	
	// Check PDP
	if( !(PAGEDIRPTR(VAddr >> 30) & 1) )
	{
		tmp = MM_AllocPhys();
		if(!tmp)	return 0;
		PAGEDIRPTR(VAddr >> 30) = tmp | 3;
		memset( &PAGEDIR( (VAddr>>30)<<9 ), 0, 4096 );
	}
	
	// Check Page Dir
	if( !(PAGEDIR(VAddr >> 21) & 1) )
	{
		tmp = MM_AllocPhys();
		if(!tmp)	return 0;
		PAGEDIR(VAddr >> 21) = tmp | 3;
		memset( &PAGETABLE( (VAddr>>21)<<9 ), 0, 4096 );
	}
	
	// Check if this virtual address is already mapped
	if( PAGETABLE(VAddr >> PTAB_SHIFT) & 1 )
		return 0;
	
	PAGETABLE(VAddr >> PTAB_SHIFT) = PAddr | 3;
	
	Log("MM_Map: RETURN 1");
	
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
}

/**
 * \brief Allocate a block of memory at the specified virtual address
 */
tPAddr MM_Allocate(tVAddr VAddr)
{
	tPAddr	ret;
	
	Log("MM_Allocate: (VAddr=%x)", VAddr);
	Log("MM_Allocate: MM_AllocPhys()");
	ret = MM_AllocPhys();
	Log("MM_Allocate: ret = %x", ret);
	if(!ret)	return 0;
	
	if( !MM_Map(VAddr, ret) )
	{
		Warning("MM_Allocate: Unable to map", ret);
		MM_DerefPhys(ret);
		return 0;
	}
	
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
 * \brief Get the physical address of a virtual location
 */
tPAddr MM_GetPhysAddr(tVAddr Addr)
{
	Log("MM_GetPhysAddr: (Addr=0x%x)", Addr);
	if( !(PAGEMAPLVL4(Addr >> 39) & 1) )
		return 0;
	Log(" MM_GetPhysAddr: PDP Valid");
	if( !(PAGEDIRPTR(Addr >> 30) & 1) )
		return 0;
	Log(" MM_GetPhysAddr: PD Valid");
	if( !(PAGEDIR(Addr >> 21) & 1) )
		return 0;
	Log(" MM_GetPhysAddr: PT Valid");
	if( !(PAGETABLE(Addr >> PTAB_SHIFT) & 1) )
		return 0;
	Log(" MM_GetPhysAddr: Page Valid");
	
	return (PAGETABLE(Addr >> PTAB_SHIFT) & ~0xFFF) | (Addr & 0xFFF);
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
	return 0;
}

tVAddr MM_NewKStack(void)
{
	return 0;
}
