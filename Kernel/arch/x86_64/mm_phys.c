/*
 * Acess2 x86_64 Port
 * 
 * Physical Memory Manager
 */
#include <acess.h>
#include <mboot.h>
//#include <mm_phys.h>

enum eMMPhys_Ranges
{
	MM_PHYS_16BIT,	// Does anything need this?
	MM_PHYS_20BIT,	// Real-Mode
	MM_PHYS_24BIT,	// ISA DMA
	MM_PHYS_32BIT,	// x86 Hardware
	MM_PHYS_MAX,	// Doesn't care
	NUM_MM_PHYS_RANGES
};

// === GLOBALS ===
tSpinlock	glPhysicalPages;
Uint64	*gaSuperBitmap;	// 1 bit = 64 Pages
Uint64	*gaPrimaryBitmap;	// 1 bit = 64 Pages
Uint32	*gaiPageReferences;	// Reference Counts
tPAddr	giFirstFreePage;	// First possibly free page
Uint64	giPhysRangeFree[NUM_MM_PHYS_RANGES];	// Number of free pages in each range
Uint64	giPhysRangeFirst[NUM_MM_PHYS_RANGES];	// First free page in each range
Uint64	giPhysRangeLast[NUM_MM_PHYS_RANGES];	// Last free page in each range
Uint64	giMaxPhysPage = 0;	// Maximum Physical page

// === CODE ===
void MM_InitPhys(tMBoot_Info *MBoot)
{
	tMBoot_MMapEnt	*mmapStart;
	tMBoot_MMapEnt	*ent;
	Uint64	maxAddr = 0;
	
	// Scan the physical memory map
	mmapStart = (void *)( KERNEL_BASE | MBoot->MMapAddr );
	ent = mmapStart;
	while( (Uint)ent < (Uint)mmapStart + MBoot->MMapLength )
	{
		// Adjust for the size of the entry
		ent->Size += 4;
		
		// If entry is RAM and is above `maxAddr`, change `maxAddr`
		if(ent->Type == 1 && ent->Base + ent->Length > maxAddr)
			maxAddr = ent->Base + ent->Length;
		// Go to next entry
		ent = (tMBoot_MMapEnt *)( (Uint)ent + ent->Size );
	}
	
	if(maxAddr == 0) {
		giMaxPhysPage = (MBoot->HighMem >> 2) + 256;	// HighMem is a kByte value
	}
	else {
		giMaxPhysPage = maxAddr >> 12;
	}
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
	
	LOCK(&glPhysicalPages);
	
	// Check if the range actually has any free pages
	while(giPhysRangeFree[rangeID] == 0 && rangeID)
		rangeID --;
	
	// What the? Oh, man. No free pages
	if(giPhysRangeFree[rangeID] == 0) {
		RELEASE(&glPhysicalPages);
		// TODO: Page out
		// ATM. Just Warning
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
		RELEASE(&glPhysicalPages);
		// TODO: Page out
		// ATM. Just Warning
		Log_Warning("Arch",
			"Out of memory (unable to fulfil request for %i pages)",
			Num
			);
		return 0;
	}
	
	// Mark pages as allocated
	addr -= Num;
	for( i = 0; i < Num; i++ )
	{
		gaiPageReferences[addr] = 1;
		gaPrimaryBitmap[addr>>6] |= 1 << (addr & 63);
		if( gaPrimaryBitmap[addr>>6] == -1 )
			gaSuperBitmap[addr>>12] |= 1 << ((addr >> 6) & 64);
		
		     if(addr >> 32)	rangeID = MM_PHYS_MAX;
		else if(addr >> 24)	rangeID = MM_PHYS_32BIT;
		else if(addr >> 20)	rangeID = MM_PHYS_24BIT;
		else if(addr >> 16)	rangeID = MM_PHYS_20BIT;
		else if(addr >> 0)	rangeID = MM_PHYS_16BIT;
		giPhysRangeFree[ rangeID ] --;
	}
	
	RELEASE(&glPhysicalPages);
	return addr;
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
	if( PAddr >> 12 > giMaxPhysPage )	return ;
	gaiPageReferences[ PAddr >> 12 ] ++;
	
	gaPrimaryBitmap[PAddr >> 18] |= 1 << ((PAddr>>12) & 63);
	if( gaPrimaryBitmap[PAddr >> 18] == -1 )
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
		gaPrimaryBitmap[PAddr >> 18] &= ~(1 << ((PAddr >> 12) & 63));
		gaSuperBitmap[PAddr >> 24] &= ~(1 << ((PAddr >> 18) & 64));
	}
}
