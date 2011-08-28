/*
 * Acess2 Core
 * 
 * include/tpl_mm_phys_bitmap.h
 * Physical Memory Manager Template
 */
#define DEBUG	0

/*
 * Bitmap Edition
 * 
 * Uses 4.125+PtrSize bytes per page
 */

#define MM_PAGE_REFCOUNTS	MM_PMM_BASE
#define MM_PAGE_NODES	(MM_PMM_BASE+(MM_MAXPHYSPAGE*sizeof(Uint32)))
#define MM_PAGE_BITMAP	(MM_PAGE_NODES+(MM_MAXPHYSPAGE*sizeof(void*)))

// === PROTOTYPES ===
//void	MM_InitPhys_Multiboot(tMBoot_Info *MBoot);
//tPAddr	MM_AllocPhysRange(int Num, int Bits);
//tPAddr	MM_AllocPhys(void);
//void	MM_RefPhys(tPAddr PAddr);
//void	MM_DerefPhys(tPAddr PAddr);
 int	MM_int_GetRangeID( tPAddr Addr );
 int	MM_int_GetMapEntry( void *Data, int Index, tPAddr *Start, tPAddr *Length );
void	MM_Tpl_InitPhys(int MaxRAMPage, void *MemoryMap);

// === GLOBALS ===
tMutex	glPhysicalPages;
void	**gapPageNodes = (void*)MM_PAGE_NODES;	//!< Associated VFS Node for each page
Uint32	*gaiPageReferences = (void*)MM_PAGE_REFCOUNTS;	// Reference Counts
Uint32	*gaPageBitmaps = (void*)MM_PAGE_BITMAP;	// Used bitmap (1 == avail)
Uint64	giMaxPhysPage = 0;	// Maximum Physical page
 int	gbPMM_Init = 0;
 int	gaiPhysRangeFirstFree[MM_NUM_RANGES];
 int	gaiPhysRangeLastFree[MM_NUM_RANGES];
 int	gaiPhysRangeNumFree[MM_NUM_RANGES];

// === CODE ===
/**
 * \brief Initialise the physical memory manager with a passed memory map
 */
void MM_Tpl_InitPhys(int MaxRAMPage, void *MemoryMap)
{
	 int	mapIndex = 0;
	tPAddr	rangeStart, rangeLen;

	if( MM_PAGE_BITMAP + (MM_MAXPHYSPAGE/8) > MM_PMM_END ) {
		Log_KernelPanic("PMM", "Config Error, PMM cannot fit data in allocated range");
	}

	giMaxPhysPage = MaxRAMPage;

	while( MM_int_GetMapEntry(MemoryMap, mapIndex++, &rangeStart, &rangeLen) )
	{
		tVAddr	bitmap_page;
		rangeStart /= PAGE_SIZE;
		rangeLen /= PAGE_SIZE;

		bitmap_page = (tVAddr)&gaPageBitmaps[rangeStart/32];
		bitmap_page &= ~(PAGE_SIZE-1);

		// Only need to allocate bitmaps
		if( !MM_GetPhysAddr( bitmap_page ) ) {
			if( MM_Allocate( bitmap_page ) ) {
				Log_KernelPanic("PMM", "Out of memory during init, this is bad");
				return ;
			}
			memset( (void*)bitmap_page, 0, rangeStart/8 & ~(PAGE_SIZE-1) );
		}
		
		// Align to 32 pages
		for( ; (rangeStart & 31) && rangeLen > 0; rangeStart++, rangeLen-- ) {
			gaPageBitmaps[rangeStart / 32] |= 1 << (rangeStart&31);
		}
		// Mark blocks of 32 as avail
		for( ; rangeLen > 31; rangeStart += 32, rangeLen -= 32 ) {
			gaPageBitmaps[rangeStart / 32] = -1;
		}
		// Mark the tail
		for( ; rangeLen > 0; rangeStart ++, rangeLen -- ) {
			gaPageBitmaps[rangeStart / 32] |= 1 << (rangeStart&31);
		}
	}

	gbPMM_Init = 1;

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

	// Get range ID	
	if( MaxBits <= 0 || MaxBits >= 64 )	// Speedup for the common case
		rangeID = MM_RANGE_MAX;
	else
		rangeID = MM_int_GetRangeID( (1LL << MaxBits) - 1 );
	
	Mutex_Acquire(&glPhysicalPages);
	
	// Check if the range actually has any free pages
	while(gaiPhysRangeNumFree[rangeID] == 0 && rangeID)
		rangeID --;
	
	LOG("rangeID = %i", rangeID);

	// Check if there is enough in the range
	if(gaiPhysRangeNumFree[rangeID] >= Pages)
	{
		LOG("{%i,0x%x -> 0x%x}",
			giPhysRangeFree[rangeID],
			giPhysRangeFirst[rangeID], giPhysRangeLast[rangeID]
			);
		// Do a cheap scan, scanning upwards from the first free page in
		// the range
		nFree = 0;
		addr = gaiPhysRangeFirstFree[ rangeID ];
		while( addr <= gaiPhysRangeLastFree[ rangeID ] )
		{
			#if USE_SUPER_BITMAP
			// Check the super bitmap
			if( gaSuperBitmap[addr / (32*32)] == 0 )
			{
				LOG("nFree = %i = 0 (super) (0x%x)", nFree, addr);
				nFree = 0;
				addr += 1LL << (6+6);
				addr &= ~0xFFF;	// (1LL << 6+6) - 1
				continue;
			}
			#endif
			// Check page block (32 pages)
			if( gaPageBitmaps[addr / 32] == 0) {
				LOG("nFree = %i = 0 (main) (0x%x)", nFree, addr);
				nFree = 0;
				addr += 1LL << (6);
				addr &= ~0x3F;
				continue;
			}
			// Check individual page
			if( !(gaPageBitmaps[addr / 32] & (1LL << (addr & 31))) )
			{
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
		addr = gaiPhysRangeLastFree[ rangeID ];
		// TODO
		Mutex_Release(&glPhysicalPages);
		// TODO: Page out
		// ATM. Just Warning
		Warning(" MM_AllocPhysRange: Out of memory (unable to fulfil request for %i pages)", Pages);
		Log_Warning("PMM",
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
		// Mark as used
		gaPageBitmaps[addr / 32] &= ~(1 << (addr & 31));
		// Maintain first possible free
		rangeID = MM_int_GetRangeID(addr * PAGE_SIZE);
		gaiPhysRangeNumFree[ rangeID ] --;
		if(addr == gaiPhysRangeFirstFree[ rangeID ])
			gaiPhysRangeFirstFree[ rangeID ] += 1;
	
		// Mark as referenced if the reference count page is valid	
		if(MM_GetPhysAddr( (tVAddr)&gaiPageReferences[addr] )) {
			gaiPageReferences[addr] = 1;
		}
	}
	ret = addr;	// Save the return address
	
	#if USE_SUPER_BITMAP
	// Update super bitmap
	Pages += addr & (32-1);
	addr &= ~(32-1);
	Pages = (Pages + (32-1)) & ~(32-1);
	for( i = 0; i < Pages/32; i++ )
	{
		if( gaPageBitmaps[ addr / 32 ] + 1 == 0 )
			gaSuperBitmap[addr / (32*32)] |= 1LL << ((addr / 32) & 31);
	}
	#endif
	
	Mutex_Release(&glPhysicalPages);
	LEAVE('x', ret << 12);
	return ret << 12;
}

/**
 * \brief Allocate a single physical page, with no preference as to address size.
 */
tPAddr MM_AllocPhys(void)
{
	 int	i;

	if( !gbPMM_Init )
	{	
		// Hack to allow allocation during setup
		for(i = 0; i < NUM_STATIC_ALLOC; i++) {
			if( gaiStaticAllocPages[i] ) {
				tPAddr	ret = gaiStaticAllocPages[i];
				gaiStaticAllocPages[i] = 0;
				Log("MM_AllocPhys: Return %x, static alloc %i", ret, i);
				return ret;
			}
		}
		
		tPAddr	ret = 0;
		for( ret = 0; ret < giMaxPhysPage; ret ++ )
		{
			if( !MM_GetPhysAddr( (tVAddr)&gaPageBitmaps[ret/32] ) ) {
				ret += PAGE_SIZE*8;
				continue ;
			}
			if( gaPageBitmaps[ret/32] == 0 ) {
				ret += 32-1;
				continue ;
			}
			if( gaPageBitmaps[ret/32] & (1 << (ret&31)) ) {
				gaPageBitmaps[ret/32] &= ~(1 << (ret&31));
				return ret * PAGE_SIZE;
			}
		}
		Log_Error("PMM", "MM_AllocPhys failed duing init");
		return 0;
	}
	
	return MM_AllocPhysRange(1, -1);
}

/**
 * \brief Reference a physical page
 */
void MM_RefPhys(tPAddr PAddr)
{
	tPAddr	page = PAddr / PAGE_SIZE;
	
	if( page >= giMaxPhysPage )	return ;
	
	if( gaPageBitmaps[ page / 32 ] & (1LL << (page&31)) )
	{
		// Allocate
		gaPageBitmaps[page / 32] &= ~(1LL << (page&31));
		#if USE_SUPER_BITMAP
		if( gaPageBitmaps[page / 32] == 0 )
			gaSuperBitmap[page / (32*32)] &= ~(1LL << ((page / 32) & 31));
		#endif
	}
	else
	{
		tVAddr	refpage = (tVAddr)&gaiPageReferences[page] & ~(PAGE_SIZE-1);
		// Reference again
		if( !MM_GetPhysAddr( refpage ) )
		{
			if( MM_Allocate(refpage) == 0 ) {
				// Out of memory, can this be resolved?
				// TODO: Reclaim memory
				Log_Error("PMM", "Out of memory (MM_RefPhys)");
				return ;
			}
			memset((void*)refpage, 0, PAGE_SIZE);
			gaiPageReferences[page] = 2;
		}
		else
			gaiPageReferences[ page ] ++;
	}
}

int MM_GetRefCount(tPAddr PAddr)
{
	PAddr >>= 12;
	if( MM_GetPhysAddr( (tVAddr)&gaiPageReferences[PAddr] ) ) {
		return gaiPageReferences[PAddr];
	}
	
	if( gaPageBitmaps[ PAddr / 32 ] & (1LL << (PAddr&31)) ) {
		return 1;
	}
	
	return 0;
}

/**
 * \brief Dereference a physical page
 */
void MM_DerefPhys(tPAddr PAddr)
{
	Uint64	page = PAddr >> 12;
	
	if( PAddr >> 12 > giMaxPhysPage )	return ;
	
	if( MM_GetPhysAddr( (tVAddr)&gaiPageReferences[page] ) )
	{
		if( gaiPageReferences[page] > 0 )
			gaiPageReferences[ page ] --;
		if( gaiPageReferences[ page ] == 0 ) {
			gaPageBitmaps[ page / 32 ] |= 1 << (page&31);
			// TODO: Catch when all pages in this range have been dereferenced
		}
	}
	else
		gaPageBitmaps[ page / 32 ] |= 1 << (page&31);
	// Clear node if needed
	if( MM_GetPhysAddr( (tVAddr)&gapPageNodes[page] ) ) {
		gapPageNodes[page] = NULL;
		// TODO: Catch when all pages in this range are not using nodes
	}
	
	// Update the free counts if the page was freed
	if( gaPageBitmaps[ page / 32 ] & (1LL << (page&31)) )
	{
		 int	rangeID;
		rangeID = MM_int_GetRangeID( PAddr );
		gaiPhysRangeNumFree[ rangeID ] ++;
		if( gaiPhysRangeFirstFree[rangeID] > page )
			gaiPhysRangeFirstFree[rangeID] = page;
		if( gaiPhysRangeLastFree[rangeID] < page )
			gaiPhysRangeLastFree[rangeID] = page;
	}

	#if USE_SUPER_BITMAP	
	// If the bitmap entry is not zero, set the bit free in the super bitmap
	if(gaPageBitmaps[ page / 32 ] != 0 ) {
		gaSuperBitmap[page / (32*32)] |= 1LL << ((page / 32) & 31);
	}
	#endif
}

int MM_SetPageNode(tPAddr PAddr, void *Node)
{
	tPAddr	page = PAddr >> 12;
	tVAddr	node_page = ((tVAddr)&gapPageNodes[page]) & ~(PAGE_SIZE-1);

	if( !MM_GetRefCount(PAddr) )	return 1;
	
	if( !MM_GetPhysAddr(node_page) ) {
		if( !MM_Allocate(node_page) )
			return -1;
		memset( (void*)node_page, 0, PAGE_SIZE );
	}

	gapPageNodes[page] = Node;
	return 0;
}

int MM_GetPageNode(tPAddr PAddr, void **Node)
{
	PAddr >>= 12;
	if( !MM_GetRefCount(PAddr) )	return 1;
	
	if( !MM_GetPhysAddr( (tVAddr)&gapPageNodes[PAddr] ) ) {
		*Node = NULL;
		return 0;
	}

	*Node = gapPageNodes[PAddr];
	return 0;
}

