/*
 * Acess2 x86_64 Port
 * 
 * Physical Memory Manager
 */
#include <acess.h>
#include <mboot.h>
#include <mm_virt.h>

enum eMMPhys_Ranges
{
	MM_PHYS_16BIT,	// Does anything need this?
	MM_PHYS_20BIT,	// Real-Mode
	MM_PHYS_24BIT,	// ISA DMA
	MM_PHYS_32BIT,	// x86 Hardware
	MM_PHYS_MAX,	// Doesn't care
	NUM_MM_PHYS_RANGES
};

// === IMPORTS ===
extern void	gKernelEnd;

// === GLOBALS ===
tSpinlock	glPhysicalPages;
Uint64	*gaSuperBitmap;	// 1 bit = 64 Pages
Uint32	*gaiPageReferences = (void*)MM_PAGE_COUNTS;	// Reference Counts
tPAddr	giFirstFreePage;	// First possibly free page
Uint64	giPhysRangeFree[NUM_MM_PHYS_RANGES];	// Number of free pages in each range
Uint64	giPhysRangeFirst[NUM_MM_PHYS_RANGES];	// First free page in each range
Uint64	giPhysRangeLast[NUM_MM_PHYS_RANGES];	// Last free page in each range
Uint64	giMaxPhysPage = 0;	// Maximum Physical page

// === CODE ===
void MM_InitPhys_Multiboot(tMBoot_Info *MBoot)
{
	tMBoot_MMapEnt	*mmapStart;
	tMBoot_MMapEnt	*ent;
	Uint64	maxAddr = 0;
	 int	numPages;
	 int	superPages;
	
	Log("MM_InitPhys_Multiboot: (MBoot=%p)", MBoot);
	
	// Scan the physical memory map
	// Looking for the top of physical memory
	mmapStart = (void *)( KERNEL_BASE | MBoot->MMapAddr );
	Log(" MM_InitPhys_Multiboot: mmapStart = %p", mmapStart);
	ent = mmapStart;
	while( (Uint)ent < (Uint)mmapStart + MBoot->MMapLength )
	{
		// Adjust for the size of the entry
		ent->Size += 4;
		Log(" MM_InitPhys_Multiboot: ent={Type:%i,Base:0x%x,Length:%x",
			ent->Type, ent->Base, ent->Length);
		
		// If entry is RAM and is above `maxAddr`, change `maxAddr`
		if(ent->Type == 1 && ent->Base + ent->Length > maxAddr)
			maxAddr = ent->Base + ent->Length;
		
		// Go to next entry
		ent = (tMBoot_MMapEnt *)( (Uint)ent + ent->Size );
	}
	
	// Did we find a valid end?
	if(maxAddr == 0) {
		// No, darn, let's just use the HighMem hack
		giMaxPhysPage = (MBoot->HighMem >> 2) + 256;	// HighMem is a kByte value
	}
	else {
		// Goodie, goodie gumdrops
		giMaxPhysPage = maxAddr >> 12;
	}
	Log(" MM_InitPhys_Multiboot: giMaxPhysPage = 0x%x", giMaxPhysPage);
	
	// Find a contigous section of memory to hold it in
	// - Starting from the end of the kernel
	// - We also need a region for the super bitmap
	superPages = ((giMaxPhysPage+64*8-1)/(64*8) + 0xFFF) >> 12;
	numPages = (giMaxPhysPage + 7) * sizeof(*gaiPageReferences);
	numPages = (numPages + 0xFFF) >> 12;
	Log(" MM_InitPhys_Multiboot: numPages = %i", numPages);
	if(maxAddr == 0)
	{
		// Ok, naieve allocation, just put it after the kernel
		tVAddr	vaddr = MM_PAGE_COUNTS;
		tPAddr	paddr = (tPAddr)&gKernelEnd - KERNEL_BASE;
		while(numPages --)
		{
			MM_Map(vaddr, paddr);
			vaddr += 0x1000;
			paddr += 0x1000;
		}
		// Allocate the super bitmap
		gaSuperBitmap = (void*) MM_MapHWPages(
			paddr,
			((giMaxPhysPage+64*8-1)/(64*8) + 0xFFF) >> 12
			);
	}
	// Scan for a nice range
	else
	{
		 int	todo = numPages + superPages;
		tPAddr	paddr = 0;
		tVAddr	vaddr = MM_PAGE_COUNTS;
		// Scan!
		for(
			ent = mmapStart;
			(Uint)ent < (Uint)mmapStart + MBoot->MMapLength;
			ent = (tMBoot_MMapEnt *)( (Uint)ent + ent->Size )
			)
		{
			 int	avail;
			
			// RAM only please
			if( ent->Type != 1 )
				continue;
			
			// Let's not put it below the kernel, shall we?
			if( ent->Base + ent->Size < (tPAddr)&gKernelEnd )
				continue;
			
			// Check if the kernel is in this range
			if( ent->Base < (tPAddr)&gKernelEnd - KERNEL_BASE && ent->Base + ent->Size > (tPAddr)&gKernelEnd )
			{
				avail = (ent->Size-((tPAddr)&gKernelEnd-KERNEL_BASE-ent->Base)) >> 12;
				paddr = (tPAddr)&gKernelEnd - KERNEL_BASE;
			}
			// No? then we can use all of the block
			else
			{
				avail = ent->Size >> 12;
				paddr = ent->Base;
			}
			
			// Map
			// - Counts
			if( todo ) {
				 int	i, max;
				max = todo < avail ? todo : avail;
				for( i = 0; i < max; i ++ )
				{
					MM_Map(vaddr, paddr);
					todo --;
					vaddr += 0x1000;
					paddr += 0x1000;
				}
				// Alter the destination address when needed
				if(todo == superPages)
					vaddr = MM_PAGE_SUPBMP;
			}
			else
				break;
		}
	}
	
	// Fill the bitmaps
	// - initialise to one, then clear the avaliable areas
	memset(gaSuperBitmap, -1, (giMaxPhysPage+64*8-1)/(64*8));
	memset(gaiPageReferences, -1, giMaxPhysPage*sizeof(*gaiPageReferences));
	// - Clear all Type=1 areas
	for(
		ent = mmapStart;
		(Uint)ent < (Uint)mmapStart + MBoot->MMapLength;
		ent = (tMBoot_MMapEnt *)( (Uint)ent + ent->Size )
		)
	{
		// Check if the type is RAM
		if(ent->Type != 1)	continue;
		// Clear the range
		memset(
			&gaiPageReferences[ ent->Base >> 12 ],
			0,
			(ent->Size>>12)*sizeof(*gaiPageReferences)
			);
	}
	
	// Reference the used pages
	// - Kernel
	// - Reference Counts and Bitmap
}

/**
 * \brief Allocate a contiguous range of physical pages with a maximum
 *        bit size of \a Bits
 * \param Num	Number of pages to allocate
 * \param Bits	Maximum size of the physical address
 * \note If \a Bits is <= 0, any sized address is used (with preference
 *       to higher addresses)
 */
tPAddr MM_AllocPhysRange(int Num, int Bits)
{
	tPAddr	addr;
	 int	rangeID;
	 int	nFree = 0, i;
	
	Log("MM_AllocPhysRange: (Num=%i,Bits=%i)", Num, Bits);
	
	if( Bits <= 0 )	// Speedup for the common case
		rangeID = MM_PHYS_MAX;
	else if( Bits > 32 )
		rangeID = MM_PHYS_MAX;
	else if( Bits > 24 )
		rangeID = MM_PHYS_32BIT;
	else if( Bits > 20 )
		rangeID = MM_PHYS_24BIT;
	else if( Bits > 16 )
		rangeID = MM_PHYS_20BIT;
	else
		rangeID = MM_PHYS_16BIT;
	
	Log(" MM_AllocPhysRange: rangeID = %i", rangeID);
	
	LOCK(&glPhysicalPages);
	Log(" MM_AllocPhysRange: i has lock");
	
	// Check if the range actually has any free pages
	while(giPhysRangeFree[rangeID] == 0 && rangeID)
		rangeID --;
	
	Log(" MM_AllocPhysRange: rangeID = %i", rangeID);
	
	// What the? Oh, man. No free pages
	if(giPhysRangeFree[rangeID] == 0) {
		RELEASE(&glPhysicalPages);
		// TODO: Page out
		// ATM. Just Warning
		Warning(" MM_AllocPhysRange: Out of free pages");
		Log_Warning("Arch",
			"Out of memory (unable to fulfil request for %i pages), zero remaining",
			Num
			);
		return 0;
	}
	
	// Check if there is enough in the range
	if(giPhysRangeFree[rangeID] >= Num)
	{
		// Do a cheap scan, scanning upwards from the first free page in
		// the range
		nFree = 1;
		addr = giPhysRangeFirst[ rangeID ];
		while( addr < giPhysRangeLast[ rangeID ] )
		{
			// Check the super bitmap
			if( gaSuperBitmap[addr >> (6+6)] == -1 ) {
				nFree = 0;
				addr += 1 << (6+6);
				addr &= (1 << (6+6)) - 1;
				continue;
			}
			// Check page block (64 pages)
			if( gaSuperBitmap[addr >> (6+6)] & (1 << (addr>>6)&63)) {
				nFree = 0;
				addr += 1 << (12+6);
				addr &= (1 << (12+6)) - 1;
				continue;
			}
			// Check individual page
			if( gaiPageReferences[addr] ) {
				nFree = 0;
				addr ++;
				continue;
			}
			nFree ++;
			addr ++;
			if(nFree == Num)
				break;
		}
		// If we don't find a contiguous block, nFree will not be equal
		// to Num, so we set it to zero and do the expensive lookup.
		if(nFree != Num)	nFree = 0;
	}
	
	if( !nFree )
	{
		// Oops. ok, let's do an expensive check (scan down the list
		// until a free range is found)
		nFree = 1;
		addr = giPhysRangeLast[ rangeID ];
		// TODO
		RELEASE(&glPhysicalPages);
		// TODO: Page out
		// ATM. Just Warning
		Log_Warning("Arch",
			"Out of memory (unable to fulfil request for %i pages)",
			Num
			);
		return 0;
	}
	Log(" MM_AllocPhysRange: nFree = %i, addr = 0x%08x", nFree, addr);
	
	// Mark pages as allocated
	addr -= Num;
	for( i = 0; i < Num; i++ )
	{
		gaiPageReferences[addr] = 1;
		
		     if(addr >> 32)	rangeID = MM_PHYS_MAX;
		else if(addr >> 24)	rangeID = MM_PHYS_32BIT;
		else if(addr >> 20)	rangeID = MM_PHYS_24BIT;
		else if(addr >> 16)	rangeID = MM_PHYS_20BIT;
		else if(addr >> 0)	rangeID = MM_PHYS_16BIT;
		giPhysRangeFree[ rangeID ] --;
	}
	// Fill super bitmap
	Num += addr & (64-1);
	addr &= ~(64-1);
	Num = (Num + (64-1)) & ~(64-1);
	for( i = 0; i < Num/64; i++ )
	{
		 int	j, bFull = 1;
		for( j = 0; j < 64; j ++ ) {
			if( gaiPageReferences[addr+i*64+j] ) {
				bFull = 0;
				break;
			}
		}
		if( bFull )
			gaSuperBitmap[addr>>12] |= 1 << ((addr >> 6) & 64);
	}
	
	RELEASE(&glPhysicalPages);
	return addr << 12;
}

/**
 * \brief Allocate a single physical page, with no preference as to address
 *        size.
 */
tPAddr MM_AllocPhys(void)
{
	return MM_AllocPhysRange(1, -1);
}

/**
 * \brief Reference a physical page
 */
void MM_RefPhys(tPAddr PAddr)
{
	 int	bIsFull, j;
	if( PAddr >> 12 > giMaxPhysPage )	return ;
	gaiPageReferences[ PAddr >> 12 ] ++;
	
	bIsFull = 1;
	for( j = 0; j < 64; j++ ) {
		if( gaiPageReferences[ PAddr >> 12 ] == 0 ) {
			bIsFull = 0;
			break;
		}
	}
	if( bIsFull )
		gaSuperBitmap[PAddr >> 24] |= 1 << ((PAddr >> 18) & 64);
}

/**
 * \brief Dereference a physical page
 */
void MM_DerefPhys(tPAddr PAddr)
{
	if( PAddr >> 12 > giMaxPhysPage )	return ;
	gaiPageReferences[ PAddr >> 12 ] --;
	if( gaiPageReferences[ PAddr >> 12 ] )
	{
		gaSuperBitmap[PAddr >> 24] &= ~(1 << ((PAddr >> 18) & 64));
	}
}
