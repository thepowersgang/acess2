/*
 * Acess2 x86_64 Port
 * 
 * Physical Memory Manager
 */
#define DEBUG	0
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
extern char	gKernelBase[];
extern char	gKernelEnd[];

// === PROTOTYPES ===
void	MM_InitPhys_Multiboot(tMBoot_Info *MBoot);
//tPAddr	MM_AllocPhysRange(int Num, int Bits);
//tPAddr	MM_AllocPhys(void);
//void	MM_RefPhys(tPAddr PAddr);
//void	MM_DerefPhys(tPAddr PAddr);
 int	MM_int_GetRangeID( tPAddr Addr );

// === GLOBALS ===
tMutex	glPhysicalPages;
Uint64	*gaSuperBitmap = (void*)MM_PAGE_SUPBMP;	// 1 bit = 64 Pages, 16 MiB per Word
Uint64	*gaMainBitmap = (void*)MM_PAGE_BITMAP;	// 1 bit = 1 Page, 256 KiB per Word
Uint64	*gaMultiBitmap = (void*)MM_PAGE_DBLBMP;	// Each bit means that the page is being used multiple times
Uint32	*gaiPageReferences = (void*)MM_PAGE_COUNTS;	// Reference Counts
tPAddr	giFirstFreePage;	// First possibly free page
Uint64	giPhysRangeFree[NUM_MM_PHYS_RANGES];	// Number of free pages in each range
Uint64	giPhysRangeFirst[NUM_MM_PHYS_RANGES];	// First free page in each range
Uint64	giPhysRangeLast[NUM_MM_PHYS_RANGES];	// Last free page in each range
Uint64	giMaxPhysPage = 0;	// Maximum Physical page
// Only used in init, allows the init code to provide pages for use by
// the allocator before the bitmaps exist.
// 3 entries because the are three calls to MM_AllocPhys in MM_Map
#define NUM_STATIC_ALLOC	3
tPAddr	gaiStaticAllocPages[NUM_STATIC_ALLOC] = {0};

// === CODE ===
/**
 * \brief Initialise the physical memory map using a Multiboot 1 map
 */
void MM_InitPhys_Multiboot(tMBoot_Info *MBoot)
{
	tMBoot_MMapEnt	*mmapStart;
	tMBoot_MMapEnt	*ent;
	Uint64	maxAddr = 0;
	 int	numPages, superPages;
	 int	i;
	Uint64	base, size;
	tVAddr	vaddr;
	tPAddr	paddr, firstFreePage;
	
	ENTER("pMBoot=%p", MBoot);
	
	// Scan the physical memory map
	// Looking for the top of physical memory
	mmapStart = (void *)( KERNEL_BASE | MBoot->MMapAddr );
	LOG("mmapStart = %p", mmapStart);
	ent = mmapStart;
	while( (Uint)ent < (Uint)mmapStart + MBoot->MMapLength )
	{
		// Adjust for the size of the entry
		ent->Size += 4;
		LOG("ent={Type:%i,Base:0x%x,Length:%x",
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
	LOG("giMaxPhysPage = 0x%x", giMaxPhysPage);
	
	// Find a contigous section of memory to hold it in
	// - Starting from the end of the kernel
	// - We also need a region for the super bitmap
	superPages = ((giMaxPhysPage+64*8-1)/(64*8) + 0xFFF) >> 12;
	numPages = (giMaxPhysPage + 7) / 8;
	numPages = (numPages + 0xFFF) >> 12;
	LOG("numPages = %i, superPages = %i", numPages, superPages);
	if(maxAddr == 0)
	{
		 int	todo = numPages*2 + superPages;
		// Ok, naieve allocation, just put it after the kernel
		// - Allocated Bitmap
		vaddr = MM_PAGE_BITMAP;
		paddr = (tPAddr)&gKernelEnd - KERNEL_BASE;
		while(todo )
		{
			// Allocate statics
			for( i = 0; i < NUM_STATIC_ALLOC; i++) {
				if(gaiStaticAllocPages[i] != 0)	continue;
				gaiStaticAllocPages[i] = paddr;
				paddr += 0x1000;
			}
			
			MM_Map(vaddr, paddr);
			vaddr += 0x1000;
			paddr += 0x1000;
			
			todo --;
			
			if( todo == numPages + superPages )
				vaddr = MM_PAGE_DBLBMP;
			if( todo == superPages )
				vaddr = MM_PAGE_SUPBMP;
		}
	}
	// Scan for a nice range
	else
	{
		 int	todo = numPages*2 + superPages;
		paddr = 0;
		vaddr = MM_PAGE_BITMAP;
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
			if( ent->Base + ent->Size < (tPAddr)&gKernelBase )
				continue;
			
			LOG("%x <= %x && %x > %x",
				ent->Base, (tPAddr)&gKernelBase,
				ent->Base + ent->Size, (tPAddr)&gKernelEnd - KERNEL_BASE
				);
			// Check if the kernel is in this range
			if( ent->Base <= (tPAddr)&gKernelBase
			&& ent->Base + ent->Length > (tPAddr)&gKernelEnd - KERNEL_BASE )
			{
				avail = ent->Length >> 12;
				avail -= ((tPAddr)&gKernelEnd - KERNEL_BASE - ent->Base) >> 12;
				paddr = (tPAddr)&gKernelEnd - KERNEL_BASE;
			}
			// No? then we can use all of the block
			else
			{
				avail = ent->Length >> 12;
				paddr = ent->Base;
			}
			
			Log(" MM_InitPhys_Multiboot: paddr=0x%x, avail=%i", paddr, avail);
			
			// Map
			while( todo && avail --)
			{
				// Static Allocations
				for( i = 0; i < NUM_STATIC_ALLOC && avail; i++) {
					if(gaiStaticAllocPages[i] != 0)	continue;
					gaiStaticAllocPages[i] = paddr;
					paddr += 0x1000;
					avail --;
				}
				if(!avail)	break;
				
				// Map
				MM_Map(vaddr, paddr);
				todo --;
				vaddr += 0x1000;
				paddr += 0x1000;
				
				// Alter the destination address when needed
				if(todo == superPages+numPages)
					vaddr = MM_PAGE_DBLBMP;
				if(todo == superPages)
					vaddr = MM_PAGE_SUPBMP;
			}
			
			// Fast quit if there's nothing left to allocate
			if( !todo )		break;
		}
	}
	// Save the current value of paddr to simplify the allocation later
	firstFreePage = paddr;
	
	LOG("Clearing multi bitmap");
	// Fill the bitmaps
	memset(gaMultiBitmap, 0, (numPages<<12)/8);
	// - initialise to one, then clear the avaliable areas
	memset(gaMainBitmap, -1, (numPages<<12)/8);
	memset(gaSuperBitmap, -1, (numPages<<12)/(8*64));
	LOG("Setting main bitmap");
	// - Clear all Type=1 areas
	LOG("Clearing valid regions");
	for(
		ent = mmapStart;
		(Uint)ent < (Uint)mmapStart + MBoot->MMapLength;
		ent = (tMBoot_MMapEnt *)( (Uint)ent + ent->Size )
		)
	{
		// Check if the type is RAM
		if(ent->Type != 1)	continue;
		
		// Main bitmap
		base = ent->Base >> 12;
		size = ent->Size >> 12;
		
		if(base & 63) {
			Uint64	val = -1LL << (base & 63);
			gaMainBitmap[base / 64] &= ~val;
			size -= (base & 63);
			base += 64 - (base & 63);
		}
		memset( &gaMainBitmap[base / 64], 0, size/8 );
		if( size & 7 ) {
			Uint64	val = -1LL << (size & 7);
			val <<= (size/8)&7;
			gaMainBitmap[base / 64] &= ~val;
		}
		
		// Super Bitmap
		base = ent->Base >> 12;
		size = ent->Size >> 12;
		size = (size + (base & 63) + 63) >> 6;
		base = base >> 6;
		if(base & 63) {
			Uint64	val = -1LL << (base & 63);
			gaSuperBitmap[base / 64] &= ~val;
			size -= (base & 63);
			base += 64 - (base & 63);
		}
	}
	
	// Reference the used pages
	base = (tPAddr)&gKernelBase >> 12;
	size = firstFreePage >> 12;
	memset( &gaMainBitmap[base / 64], -1, size/8 );
	if( size & 7 ) {
		Uint64	val = -1LL << (size & 7);
		val <<= (size/8)&7;
		gaMainBitmap[base / 64] |= val;
	}
	
	// Free the unused static allocs
	for( i = 0; i < NUM_STATIC_ALLOC; i++) {
		if(gaiStaticAllocPages[i] != 0)
			continue;
		gaMainBitmap[ gaiStaticAllocPages[i] >> (12+6) ]
			&= ~(1LL << ((gaiStaticAllocPages[i]>>12)&63));
	}
	
	// Fill the super bitmap
	LOG("Filling super bitmap");
	memset(gaSuperBitmap, 0, superPages<<12);
	for( base = 0; base < (size+63)/64; base ++)
	{
		if( gaMainBitmap[ base ] + 1 == 0 )
			gaSuperBitmap[ base/64 ] |= 1LL << (base&63);
	}
	
	// Set free page counts
	for( base = 1; base < giMaxPhysPage; base ++ )
	{
		 int	rangeID;
		// Skip allocated
		if( gaMainBitmap[ base >> 6 ] & (1LL << (base&63))  )	continue;
		
		// Get range ID
		rangeID = MM_int_GetRangeID( base << 12 );
		
		// Increment free page count
		giPhysRangeFree[ rangeID ] ++;
		
		// Check for first free page in range
		if(giPhysRangeFirst[ rangeID ] == 0)
			giPhysRangeFirst[ rangeID ] = base;
		// Set last (when the last free page is reached, this won't be
		// updated anymore, hence will be correct)
		giPhysRangeLast[ rangeID ] = base;
	}
	
	LEAVE('-');
}

/**
 * \brief Allocate a contiguous range of physical pages with a maximum
 *        bit size of \a MaxBits
 * \param Pages	Number of pages to allocate
 * \param MaxBits	Maximum size of the physical address
 * \note If \a MaxBits is <= 0, any sized address is used (with preference
 *       to higher addresses)
 */
tPAddr MM_AllocPhysRange(int Pages, int MaxBits)
{
	tPAddr	addr, ret;
	 int	rangeID;
	 int	nFree = 0, i;
	
	ENTER("iPages iBits", Pages, MaxBits);
	
	if( MaxBits <= 0 || MaxBits >= 64 )	// Speedup for the common case
		rangeID = MM_PHYS_MAX;
	else
		rangeID = MM_int_GetRangeID( (1LL << MaxBits) - 1 );
	
	LOG("rangeID = %i", rangeID);
	
	Mutex_Acquire(&glPhysicalPages);
	
	// Check if the range actually has any free pages
	while(giPhysRangeFree[rangeID] == 0 && rangeID)
		rangeID --;
	
	LOG("rangeID = %i", rangeID);
	
	// What the? Oh, man. No free pages
	if(giPhysRangeFree[rangeID] == 0) {
		Mutex_Release(&glPhysicalPages);
		// TODO: Page out
		// ATM. Just Warning
		Warning(" MM_AllocPhysRange: Out of free pages");
		Log_Warning("Arch",
			"Out of memory (unable to fulfil request for %i pages), zero remaining",
			Pages
			);
		LEAVE('i', 0);
		return 0;
	}
	
	// Check if there is enough in the range
	if(giPhysRangeFree[rangeID] >= Pages)
	{
		LOG("{%i,0x%x -> 0x%x}",
			giPhysRangeFree[rangeID],
			giPhysRangeFirst[rangeID], giPhysRangeLast[rangeID]
			);
		// Do a cheap scan, scanning upwards from the first free page in
		// the range
		nFree = 0;
		addr = giPhysRangeFirst[ rangeID ];
		while( addr <= giPhysRangeLast[ rangeID ] )
		{
			//Log(" MM_AllocPhysRange: addr = 0x%x", addr);
			// Check the super bitmap
			if( gaSuperBitmap[addr >> (6+6)] + 1 == 0 ) {
				LOG("nFree = %i = 0 (super) (0x%x)", nFree, addr);
				nFree = 0;
				addr += 1LL << (6+6);
				addr &= ~0xFFF;	// (1LL << 6+6) - 1
				continue;
			}
			// Check page block (64 pages)
			if( gaMainBitmap[addr >> 6] + 1 == 0) {
				LOG("nFree = %i = 0 (main) (0x%x)", nFree, addr);
				nFree = 0;
				addr += 1LL << (6);
				addr &= ~0x3F;
				continue;
			}
			// Check individual page
			if( gaMainBitmap[addr >> 6] & (1LL << (addr & 63)) ) {
				LOG("nFree = %i = 0 (page) (0x%x)", nFree, addr);
				nFree = 0;
				addr ++;
				continue;
			}
			nFree ++;
			addr ++;
			LOG("nFree(%i) == %i (0x%x)", nFree, Pages, addr);
			if(nFree == Pages)
				break;
		}
		LOG("nFree = %i", nFree);
		// If we don't find a contiguous block, nFree will not be equal
		// to Num, so we set it to zero and do the expensive lookup.
		if(nFree != Pages)	nFree = 0;
	}
	
	if( !nFree )
	{
		// Oops. ok, let's do an expensive check (scan down the list
		// until a free range is found)
		nFree = 1;
		addr = giPhysRangeLast[ rangeID ];
		// TODO
		Mutex_Release(&glPhysicalPages);
		// TODO: Page out
		// ATM. Just Warning
		Warning(" MM_AllocPhysRange: Out of memory (unable to fulfil request for %i pages)", Pages);
		Log_Warning("Arch",
			"Out of memory (unable to fulfil request for %i pages)",
			Pages	
			);
		LEAVE('i', 0);
		return 0;
	}
	LOG("nFree = %i, addr = 0x%08x", nFree, addr);
	
	// Mark pages as allocated
	addr -= Pages;
	for( i = 0; i < Pages; i++, addr++ )
	{
		gaMainBitmap[addr >> 6] |= 1LL << (addr & 63);
		rangeID = MM_int_GetRangeID(addr << 12);
		giPhysRangeFree[ rangeID ] --;
		LOG("%x == %x", addr, giPhysRangeFirst[ rangeID ]);
		if(addr == giPhysRangeFirst[ rangeID ])
			giPhysRangeFirst[ rangeID ] += 1;
	}
	ret = addr;	// Save the return address
	
	// Update super bitmap
	Pages += addr & (64-1);
	addr &= ~(64-1);
	Pages = (Pages + (64-1)) & ~(64-1);
	for( i = 0; i < Pages/64; i++ )
	{
		if( gaMainBitmap[ addr >> 6 ] + 1 == 0 )
			gaSuperBitmap[addr>>12] |= 1LL << ((addr >> 6) & 63);
	}
	
	Mutex_Release(&glPhysicalPages);
	LEAVE('x', ret << 12);
	return ret << 12;
}

/**
 * \brief Allocate a single physical page, with no preference as to address
 *        size.
 */
tPAddr MM_AllocPhys(void)
{
	 int	i;
	
	// Hack to allow allocation during setup
	for(i = 0; i < NUM_STATIC_ALLOC; i++) {
		if( gaiStaticAllocPages[i] ) {
			tPAddr	ret = gaiStaticAllocPages[i];
			gaiStaticAllocPages[i] = 0;
			Log("MM_AllocPhys: Return %x, static alloc %i", ret, i);
			return ret;
		}
	}
	
	return MM_AllocPhysRange(1, -1);
}

/**
 * \brief Reference a physical page
 */
void MM_RefPhys(tPAddr PAddr)
{
	Uint64	page = PAddr >> 12;
	
	if( PAddr >> 12 > giMaxPhysPage )	return ;
	
	if( gaMainBitmap[ page >> 6 ] & (1LL << (page&63)) )
	{
		// Reference again
		gaMultiBitmap[ page >> 6 ] |= 1LL << (page&63);
		if( !MM_GetPhysAddr( ((tVAddr)&gaiPageReferences[ page ]) & ~0xFFF ) ) {
			if( !MM_Allocate( ((tVAddr)&gaiPageReferences[ page ]) & ~0xFFF ) ) {
				Log_Error("Arch", "Out of memory when allocating reference count page");
				return ;
			}
		}
		gaiPageReferences[ page ] ++;
	}
	else
	{
		// Allocate
		gaMainBitmap[page >> 6] |= 1LL << (page&63);
		if( gaMainBitmap[page >> 6 ] + 1 == 0 )
			gaSuperBitmap[page>> 12] |= 1LL << ((page >> 6) & 63);
	}
}

/**
 * \brief Dereference a physical page
 */
void MM_DerefPhys(tPAddr PAddr)
{
	Uint64	page = PAddr >> 12;
	
	if( PAddr >> 12 > giMaxPhysPage )	return ;
	
	if( gaMultiBitmap[ page >> 6 ] & (1LL << (page&63)) ) {
		gaiPageReferences[ page ] --;
		if( gaiPageReferences[ page ] == 1 )
			gaMultiBitmap[ page >> 6 ] &= ~(1LL << (page&63));
		if( gaiPageReferences[ page ] == 0 )
			gaMainBitmap[ page >> 6 ] &= ~(1LL << (page&63));
	}
	else
		gaMainBitmap[ page >> 6 ] &= ~(1LL << (page&63));
	
	// Update the free counts if the page was freed
	if( !(gaMainBitmap[ page >> 6 ] & (1LL << (page&63))) )
	{
		 int	rangeID;
		rangeID = MM_int_GetRangeID( PAddr );
		giPhysRangeFree[ rangeID ] ++;
		if( giPhysRangeFirst[rangeID] > page )
			giPhysRangeFirst[rangeID] = page;
		if( giPhysRangeLast[rangeID] < page )
			giPhysRangeLast[rangeID] = page;
	}
	
	// If the bitmap entry is not -1, unset the bit in the super bitmap
	if(gaMainBitmap[ page >> 6 ] + 1 != 0 ) {
		gaSuperBitmap[page >> 12] &= ~(1LL << ((page >> 6) & 63));
	}
}

/**
 * \brief Takes a physical address and returns the ID of its range
 * \param Addr	Physical address of page
 * \return Range ID from eMMPhys_Ranges
 */
int MM_int_GetRangeID( tPAddr Addr )
{
	if(Addr >> 32)
		return MM_PHYS_MAX;
	else if(Addr >> 24)
		return MM_PHYS_32BIT;
	else if(Addr >> 20)
		return MM_PHYS_24BIT;
	else if(Addr >> 16)
		return MM_PHYS_20BIT;
	else
		return MM_PHYS_16BIT;
}
