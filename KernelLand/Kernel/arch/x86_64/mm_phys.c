/*
 * Acess2 x86_64 Port
 * 
 * Physical Memory Manager
 */
#define DEBUG	0
#include <acess.h>
#include <archinit.h>
#include <pmemmap.h>
#include <mm_virt.h>
#include <debug_hooks.h>

#define TRACE_REF	0

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
//void	MM_InitPhys(int NPMemRanges, tPMemMapEnt *PMemRanges);
//tPAddr	MM_AllocPhysRange(int Num, int Bits);
//tPAddr	MM_AllocPhys(void);
//void	MM_RefPhys(tPAddr PAddr);
//void	MM_DerefPhys(tPAddr PAddr);
 int	MM_int_GetRangeID( tPAddr Addr );

// === MACROS ===
#define PAGE_ALLOC_TEST(__page) 	(gaMainBitmap[(__page)>>6] & (1ULL << ((__page)&63)))
#define PAGE_ALLOC_SET(__page)  	do{gaMainBitmap[(__page)>>6] |= (1ULL << ((__page)&63));}while(0)
#define PAGE_ALLOC_CLEAR(__page)	do{gaMainBitmap[(__page)>>6] &= ~(1ULL << ((__page)&63));}while(0)
//#define PAGE_MULTIREF_TEST(__page)	(gaMultiBitmap[(__page)>>6] & (1ULL << ((__page)&63)))
//#define PAGE_MULTIREF_SET(__page)	do{gaMultiBitmap[(__page)>>6] |= 1ULL << ((__page)&63);}while(0)
//#define PAGE_MULTIREF_CLEAR(__page)	do{gaMultiBitmap[(__page)>>6] &= ~(1ULL << ((__page)&63));}while(0)

// === GLOBALS ===
tMutex	glPhysicalPages;
Uint64	*gaSuperBitmap = (void*)MM_PAGE_SUPBMP;	// 1 bit = 64 Pages, 16 MiB per Word
Uint64	*gaMainBitmap = (void*)MM_PAGE_BITMAP;	// 1 bit = 1 Page, 256 KiB per Word
Uint64	*gaMultiBitmap = (void*)MM_PAGE_DBLBMP;	// Each bit means that the page is being used multiple times
Uint32	*gaiPageReferences = (void*)MM_PAGE_COUNTS;	// Reference Counts
void	**gapPageNodes = (void*)MM_PAGE_NODES;	// Reference Counts
tPAddr	giFirstFreePage;	// First possibly free page
Uint64	giPhysRangeFree[NUM_MM_PHYS_RANGES];	// Number of free pages in each range
Uint64	giPhysRangeFirst[NUM_MM_PHYS_RANGES];	// First free page in each range
Uint64	giPhysRangeLast[NUM_MM_PHYS_RANGES];	// Last free page in each range
Uint64	giMaxPhysPage = 0;	// Maximum Physical page
Uint64	giTotalMemorySize = 0;
// Only used in init, allows the init code to provide pages for use by
// the allocator before the bitmaps exist.
// 3 entries because the are three calls to MM_AllocPhys in MM_Map
#define NUM_STATIC_ALLOC	3
tPAddr	gaiStaticAllocPages[NUM_STATIC_ALLOC] = {0};

// === CODE ===
/**
 * \brief Initialise the physical memory map using a Multiboot 1 map
 */
void MM_InitPhys(int NPMemRanges, tPMemMapEnt *PMemRanges)
{
	Uint64	maxAddr = 0;
	 int	numPages, superPages;
	 int	i;
	Uint64	base, size;
	tVAddr	vaddr;
	tPAddr	paddr;
	
	ENTER("iNPMemRanges pPMemRanges",
		NPMemRanges, PMemRanges);
	
	// Scan the physical memory map
	// Looking for the top of physical memory
	for( i = 0; i < NPMemRanges; i ++ )
	{
		tPMemMapEnt	*ent = &PMemRanges[i];
		// Adjust for the size of the entry
		LOG("%i: ent={Type:%i,Base:0x%x,Length:%x}", i, ent->Type, ent->Start, ent->Length);
		
		// If entry is RAM and is above `maxAddr`, change `maxAddr`
		if(ent->Type == PMEMTYPE_FREE || ent->Type == PMEMTYPE_USED )
		{
			if( ent->Start + ent->Length > maxAddr)
				maxAddr = ent->Start + ent->Length;
			giTotalMemorySize += ent->Length >> 12;
		}
	}
	
	giMaxPhysPage = maxAddr >> 12;
	LOG("giMaxPhysPage = 0x%x", giMaxPhysPage);

	// Get counts of pages needed for basic structures
	superPages = ((giMaxPhysPage+64*8-1)/(64*8) + 0xFFF) >> 12;
	numPages = ((giMaxPhysPage+7)/8 + 0xFFF) >> 12;	// bytes to hold bitmap, divided up to nearest page
	LOG("numPages = %i, superPages = %i", numPages, superPages);
	
	// --- Allocate Bitmaps ---
	 int	todo = numPages*2 + superPages;
	 int	mapent = NPMemRanges-1;
	vaddr = MM_PAGE_BITMAP;
	paddr = -1;
	while( todo )
	{
		while(PMemRanges[mapent].Type != PMEMTYPE_FREE && mapent != -1)
			mapent --;
		if( paddr + 1 == 0 )
			paddr = PMemRanges[mapent].Start;
		if( mapent == -1 ) {
			// OOM During init, bad thing
			Log_KernelPanic("PMem", "Out of memory during init");
			for(;;);
		}
		
		// Ensure that the static allocation pool has pages
		for( i = 0; i < NUM_STATIC_ALLOC; i++)
		{
			if(gaiStaticAllocPages[i] == 0)
			{
				gaiStaticAllocPages[i] = paddr;
				// break to ensure we update the address correctly
				break;
			}
		}
		
		if( i == NUM_STATIC_ALLOC )
		{
			// Map
			MM_Map((void*)vaddr, paddr);
			todo --;
			
			// Update virtual pointer
			vaddr += 0x1000;
			if( todo == numPages + superPages )
				vaddr = MM_PAGE_DBLBMP;
			if( todo == superPages )
				vaddr = MM_PAGE_SUPBMP;
		}		

		// Update physical pointer
		// (underflows are detected at the top of the loop)
		paddr += 0x1000;
		if( paddr - PMemRanges[mapent].Start > PMemRanges[mapent].Length )
			mapent --;
		else {
			// NOTE: This hides some actually valid memory, but since the pmm
			// structures have an "infinite" lifetime, this is of no concequence.
			PMemRanges[mapent].Start += 0x1000;
			PMemRanges[mapent].Length -= 0x1000;
		}
	}

	PMemMap_DumpBlocks(PMemRanges, NPMemRanges);

	// Save the current value of paddr to simplify the allocation later
	giFirstFreePage = paddr;
	
	LOG("Clearing multi bitmap");
	// Fill the bitmaps (set most to "allocated")
	memset(gaMultiBitmap, 0, numPages<<12);
	memset(gaMainBitmap,  255, numPages<<12);
	// - Clear all Type=1 areas
	LOG("Clearing valid regions");
	for( i = 0; i < NPMemRanges; i ++ )
	{
		tPMemMapEnt *ent = &PMemRanges[i];
		// Check if the type is RAM
		if(ent->Type != PMEMTYPE_FREE)	continue;
		
		// Main bitmap
		base = ent->Start >> 12;
		size = ent->Length >> 12;

		LOG("%i: base=%x, size=%x", i, base, size);
		if( base % 64 + size < 64 )
		{
			Uint64	bits = (1ULL << size) - 1;
			bits <<= base % 64;
			gaMainBitmap[base / 64] &= ~bits;
		}
		else
		{
			if(base & 63)
			{
				// Keep lower bits
				Uint64	bits = (1ULL << (base & 63)) - 1;
				gaMainBitmap[base / 64] &= bits;
				
				size -= 64 - base % 64;
				base += 64 - base % 64;
			}
			LOG("%i: base=%x, size=%x", i, base, size);
			memset( &gaMainBitmap[base / 64], 0, (size/64)*8 );
			base += size & ~(64-1);
			size -= size & ~(64-1);
			LOG("%i: base=%x, size=%x", i, base, size);
			if( size & 63 )
			{
				// Unset lower bits (hence the bitwise not)
				Uint64	val = (1ULL << (size & 63)) - 1;
				gaMainBitmap[base / 64] &= ~val;
			}
		}
	}
	
	// Free the unused static allocs
	LOG("Freeing unused static allocations");
	for( i = 0; i < NUM_STATIC_ALLOC; i++)
	{
		if(gaiStaticAllocPages[i] == 0)
		{
			gaMainBitmap[ gaiStaticAllocPages[i] >> (12+6) ]
				&= ~(1LL << ((gaiStaticAllocPages[i]>>12)&63));
			gaiStaticAllocPages[i] = 0;
		}
	}
	
	// Fill the super bitmap
	LOG("Filling super bitmap");
	memset(gaSuperBitmap, 0, superPages<<12);
	int nsuperbits = giMaxPhysPage / 64;	// 64 pages per bit
	for( i = 0; i < (nsuperbits+63)/64; i ++)
	{
		if( gaMainBitmap[ i ] + 1 == 0 )
			gaSuperBitmap[ i/64 ] |= 1ULL << (i % 64);
	}
	
	// Set free page counts for each address class
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

void MM_DumpStatistics(void)
{
	// TODO: Statistics for x86_64 PMM
	Log_Warning("PMem", "TODO: Dump statistics");
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
		Log_Warning("PMem",
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
//		nFree = 1;
//		addr = giPhysRangeLast[ rangeID ];
		// TODO: Expensive Check
		Mutex_Release(&glPhysicalPages);
		// TODO: Page out
		// ATM. Just Warning
		Warning(" MM_AllocPhysRange: Out of memory (unable to fulfil request for %i pages)", Pages);
		Log_Warning("PMem",
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
		if( MM_GetPhysAddr( &gaiPageReferences[addr] ) )
			gaiPageReferences[addr] = 1;
//		Log("page %P refcount = %i", MM_GetRefCount(addr<<12)); 
		rangeID = MM_int_GetRangeID(addr << 12);
		giPhysRangeFree[ rangeID ] --;
		LOG("%x == %x", addr, giPhysRangeFirst[ rangeID ]);
		if(addr == giPhysRangeFirst[ rangeID ])
			giPhysRangeFirst[ rangeID ] += 1;
	}
	addr -= Pages;
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
	#if TRACE_REF
	Log("MM_AllocPhysRange: ret = %P (Ref %i)", ret << 12, MM_GetRefCount(ret<<12));
	#endif
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
			Log("MM_AllocPhys: Return %P, static alloc %i", ret, i);
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
	
	if( page > giMaxPhysPage )	return ;
	
	if( PAGE_ALLOC_TEST(page) )
	{
		tVAddr	ref_base = ((tVAddr)&gaiPageReferences[ page ]) & ~0xFFF;
		// Allocate reference page
		if( !MM_GetPhysAddr(&gaiPageReferences[page]) )
		{
			const int	pages_per_refpage = PAGE_SIZE/sizeof(gaiPageReferences[0]);
			 int	i;
			 int	page_base = page / pages_per_refpage * pages_per_refpage;
			if( !MM_Allocate( (void*)ref_base ) ) {
				Log_Error("PMem", "Out of memory when allocating reference count page");
				return ;
			}
			// Fill block
			Log("Allocated references for %P-%P", page_base << 12, (page_base+pages_per_refpage)<<12);
			for( i = 0; i < pages_per_refpage; i ++ ) {
				 int	pg = page_base + i;
				gaiPageReferences[pg] = !!PAGE_ALLOC_TEST(pg);
			}
		}
		gaiPageReferences[page] ++;
	}
	else
	{
		// Allocate
		PAGE_ALLOC_SET(page);
		if( gaMainBitmap[page >> 6] + 1 == 0 )
			gaSuperBitmap[page>> 12] |= 1LL << ((page >> 6) & 63);
		if( MM_GetPhysAddr( &gaiPageReferences[page] ) )
			gaiPageReferences[page] = 1;
	}

	#if TRACE_REF
	Log("MM_RefPhys: %P referenced (%i)", page << 12, MM_GetRefCount(page << 12));
	#endif
}

/**
 * \brief Dereference a physical page
 */
void MM_DerefPhys(tPAddr PAddr)
{
	Uint64	page = PAddr >> 12;
	
	if( PAddr >> 12 > giMaxPhysPage )	return ;
	
	if( MM_GetPhysAddr( &gaiPageReferences[page] ) )
	{
		gaiPageReferences[ page ] --;
		if( gaiPageReferences[ page ] == 0 )
			PAGE_ALLOC_CLEAR(page);
	}
	else
		PAGE_ALLOC_CLEAR(page);
	
	// Update the free counts if the page was freed
	if( !PAGE_ALLOC_TEST(page) )
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
	
	#if TRACE_REF
	Log("Page %P dereferenced (%i)", page << 12, MM_GetRefCount(page << 12));
	#endif
}

int MM_GetRefCount( tPAddr PAddr )
{
	PAddr >>= 12;
	
	if( PAddr > giMaxPhysPage )	return 0;

	if( MM_GetPhysAddr( &gaiPageReferences[PAddr] ) ) {
		return gaiPageReferences[PAddr];
	}

	if( PAGE_ALLOC_TEST(PAddr) )
	{
		return 1;
	}
	return 0;
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

int MM_SetPageNode(tPAddr PAddr, void *Node)
{
	tPAddr	page = PAddr >> 12;
	void	*node_page = (void*)( ((tVAddr)&gapPageNodes[page]) & ~(PAGE_SIZE-1) );

//	if( !MM_GetRefCount(PAddr) )	return 1;
	
	if( !MM_GetPhysAddr(node_page) ) {
		if( !MM_Allocate(node_page) )
			return -1;
		memset( node_page, 0, PAGE_SIZE );
	}

	gapPageNodes[page] = Node;
	return 0;
}

int MM_GetPageNode(tPAddr PAddr, void **Node)
{
//	if( !MM_GetRefCount(PAddr) )	return 1;
	PAddr >>= 12;
	
	if( !MM_GetPhysAddr( &gapPageNodes[PAddr] ) ) {
		*Node = NULL;
		return 0;
	}

	*Node = gapPageNodes[PAddr];
	return 0;
}

