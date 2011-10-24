/*
 * Acess2 Core
 * 
 * include/tpl_mm_phys.h
 * Physical Memory Manager Template
 */
#define DEBUG	0

/**
 * \file tpl_mm_phys_stack.h
 * \brief Template physical memory manager
 *
 * An extensible physical memory manager
 *
 * Usage: Requires NUM_MM_PHYS_RANGES to be set to the number of address
 * "classes" wanted.
 * MAX_PHYS_PAGES - Used to calculate structure sizes
 * PADDR_TYPE
 */

// === PROTOTYPES ===
//void	MM_InitPhys_Multiboot(tMBoot_Info *MBoot);
//tPAddr	MM_AllocPhysRange(int Num, int Bits);
//tPAddr	MM_AllocPhys(void);
//void	MM_RefPhys(tPAddr PAddr);
//void	MM_DerefPhys(tPAddr PAddr);
 int	MM_int_GetRangeID( tPAddr Addr );
 int	MM_int_IsPhysUnavail( tPageNum Page );

// === GLOBALS ===
tMutex	glPhysicalPages;
//Uint64	*gaPageStacks[NUM_MM_PHYS_RANGES];	// Page stacks
 int	giPageStackSizes[NUM_MM_PHYS_RANGES];	// Points to the first unused slot
Uint32	*gaiPageReferences = (void*)MM_PAGE_COUNTS;	// Reference Counts
Uint64	giMaxPhysPage = 0;	// Maximum Physical page

// === CODE ===
/**
 * \brief Initialise the physical memory map using a Multiboot 1 map
 */
void MM_Tpl_InitPhys(int MaxRAMPage)
{
	const int	PAGE_SIZE = 0x1000;
	const int	pagesPerPageOnStack = PAGE_SIZE / sizeof(gaPageStack[0]);
	 int	i;
	tPageNum	page;

//	ENTER("pMBoot=%p", MBoot);
	
	giMaxPhysPage = MaxRAMPage;
	
	tPAddr	page = 0;
	for( i = 0; i < NUM_MM_PHYS_RANGES; i ++ )
	{
		for( ; page < giPageRangeMax[i] && page < giMaxPhysPage; page ++ )
		{
			 int	rangeSize;

			rangeSize = MM_int_IsPhysUnavail(page);
			if( rangeSize > 0 ) {
				page += rangeSize;
				continue;
			}
			// Page is avaliable for use
	
			// Check if the page stack is allocated
			tVAddr	stack_page = &gaPageStacks[i][giPageStackSizes[i]&~pagesPerPageOnStack];
			if( !MM_GetPhysAddr( stack_page ) ) {
				MM_Map( stack_page, page*PAGE_SIZE );
			}
			else {
				gaPageStacks[i][ giPageStackSizes[i] ] = page;
				giPageStackSizes[i] ++;
			}
		}
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
	while(giPageStackSizes[rangeID] == 0 && rangeID)
		rangeID --;
	
	LOG("rangeID = %i", rangeID);
	
	// What the? Oh, man. No free pages
	if(giPageStackSizes[rangeID] == 0) {
		Mutex_Release(&glPhysicalPages);
		// TODO: Page out / attack the cache
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
