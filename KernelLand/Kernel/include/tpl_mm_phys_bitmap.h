/*
 * Acess2 Core
 * 
 * include/tpl_mm_phys_bitmap.h
 * Physical Memory Manager Template
 */
/*
 * Bitmap Edition
 * 
 * Uses 4.125+PtrSize bytes per page
 */
#include <debug_hooks.h>

#define MM_PAGE_REFCOUNTS	MM_PMM_BASE
#define MM_PAGE_NODES	(MM_PMM_BASE+(MM_MAXPHYSPAGE*sizeof(Uint32)))
#define MM_PAGE_BITMAP	(MM_PAGE_NODES+(MM_MAXPHYSPAGE*sizeof(void*)))

#define PAGE_BITMAP_FREE(__pg)	(gaPageBitmaps[(__pg)/32] & (1LL << ((__pg)&31)))
#define PAGE_BITMAP_SETFREE(__pg)	do{gaPageBitmaps[(__pg)/32] |= (1LL << ((__pg)&31));}while(0)
#define PAGE_BITMAP_SETUSED(__pg)	do{gaPageBitmaps[(__pg)/32] &= ~(1LL << ((__pg)&31));}while(0)

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
 int	giPhysFirstFree;
 int	giPhysLastFree;
 int	giPhysNumFree;

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

//	for( i = 0; i < MM_RANGE_MAX; i ++ )
//		gaiPhysRangeFirstFree[i] = -1;
	giPhysFirstFree = -1;

	while( MM_int_GetMapEntry(MemoryMap, mapIndex++, &rangeStart, &rangeLen) )
	{
		tVAddr	bitmap_page;
		
		LOG("Range %i, %P to %P", mapIndex-1, rangeStart, rangeLen);
		rangeStart /= PAGE_SIZE;
		rangeLen /= PAGE_SIZE;

		giPhysNumFree += rangeLen;

		LOG("rangeStart = 0x%x, rangeLen = 0x%x", rangeStart, rangeLen);

		if( giPhysFirstFree == -1 || giPhysFirstFree > rangeStart )
			giPhysFirstFree = rangeStart;

		if( giPhysLastFree < rangeStart + rangeLen )
			giPhysLastFree = rangeStart + rangeLen;

		LOG("giPhysFirstFree = 0x%x, giPhysLastFree = 0x%x", giPhysFirstFree, giPhysLastFree);

		bitmap_page = (tVAddr)&gaPageBitmaps[rangeStart/32];
		bitmap_page &= ~(PAGE_SIZE-1);

		// Only need to allocate bitmaps
		if( !MM_GetPhysAddr( (void*)bitmap_page ) ) {
			if( !MM_Allocate( (void*)bitmap_page ) ) {
				Log_KernelPanic("PMM", "Out of memory during init, this is bad");
				return ;
			}
//			memset( (void*)bitmap_page, 0, (rangeStart/8) & ~(PAGE_SIZE-1) );
			memset( (void*)bitmap_page, 0, PAGE_SIZE );
		}
		
		// Align to 32 pages
		for( ; (rangeStart & 31) && rangeLen > 0; rangeStart++, rangeLen-- ) {
			gaPageBitmaps[rangeStart / 32] |= 1 << (rangeStart&31);
			LOG("gaPageBitmaps[%i] = 0x%x", rangeStart/32, gaPageBitmaps[rangeStart/32]);
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

	LOG("giPhysFirstFree = 0x%x, giPhysLastFree = 0x%x", giPhysFirstFree, giPhysLastFree);
	LEAVE('-');
}

void MM_DumpStatistics(void)
{
	// TODO: PM Statistics for tpl_mm_phys_bitmap
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
	 int	nFree = 0, i;
	
	ENTER("iPages iBits", Pages, MaxBits);
	
	Mutex_Acquire(&glPhysicalPages);
	
	// Check if there is enough in the range
	if(giPhysNumFree >= Pages)
	{
		LOG("{0x%x -> 0x%x}", giPhysFirstFree, giPhysLastFree);
		// Do a cheap scan, scanning upwards from the first free page in
		// the range
		nFree = 0;
		addr = giPhysFirstFree;
		while( addr <= giPhysLastFree )
		{
			#if USE_SUPER_BITMAP
			// Check the super bitmap
			if( gaSuperBitmap[addr / (32*32)] == 0 )
			{
				LOG("nFree = %i = 0 (super) (0x%x)", nFree, addr);
				nFree = 0;
				addr += (32*32);
				addr &= ~(32*32-1);	// (1LL << 6+6) - 1
				continue;
			}
			#endif
			LOG("gaPageBitmaps[%i] = 0x%x", addr/32, gaPageBitmaps[addr/32]);
			// Check page block (32 pages)
			if( gaPageBitmaps[addr / 32] == 0) {
				LOG("nFree = %i = 0 (block) (0x%x)", nFree, addr);
				nFree = 0;
				addr += 32;
				addr &= ~31;
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
			LOG("nFree(%i) == %i (1x%x)", nFree, Pages, addr);
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
#if 0
		// Oops. ok, let's do an expensive check (scan down the list
		// until a free range is found)
		nFree = 1;
		addr = gaiPhysRangeLastFree[ rangeID ];
		// TODO
#endif
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
	LOG("nFree = %i, addr = 0x%08x", nFree, (addr-Pages) << 12);
	
	// Mark pages as allocated
	addr -= Pages;
	for( i = 0; i < Pages; i++, addr++ )
	{
		// Mark as used
		PAGE_BITMAP_SETUSED(addr);
		// Maintain first possible free
		giPhysNumFree --;
		if(addr == giPhysFirstFree)
			giPhysFirstFree += 1;
	
		LOG("if( MM_GetPhysAddr( %p ) )", &gaiPageReferences[addr]);
		// Mark as referenced if the reference count page is valid	
		if( MM_GetPhysAddr( &gaiPageReferences[addr] ) ) {
			gaiPageReferences[addr] = 1;
		}
	}
	ret = addr - Pages;	// Save the return address
	LOG("ret = %x", ret);	

	#if TRACE_ALLOCS
	LogF("MM_AllocPhysRange: %P (%i pages)\n", ret, Pages);
	if(Pages > 1) {
		LogF(" also");
		for(i = 1; i < Pages; i++)
			LogF(" %P", ret+i);
		LogF("\n");
	}
	#endif

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
			if( !MM_GetPhysAddr( &gaPageBitmaps[ret/32] ) ) {
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
	#if TRACE_ALLOCS
	Log("AllocPhys by %p", __builtin_return_address(0));
	#endif
	
	return MM_AllocPhysRange(1, -1);
}

/**
 * \brief Reference a physical page
 */
void MM_RefPhys(tPAddr PAddr)
{
	tPAddr	page = PAddr / PAGE_SIZE;
	tVAddr	refpage = (tVAddr)&gaiPageReferences[page] & ~(PAGE_SIZE-1);
	
	if( page >= giMaxPhysPage )	return ;

	if( PAGE_BITMAP_FREE(page) )
	{
		// Allocate
		PAGE_BITMAP_SETUSED(page);
		#if USE_SUPER_BITMAP
		if( gaPageBitmaps[page / 32] == 0 )
			gaSuperBitmap[page / (32*32)] &= ~(1LL << ((page / 32) & 31));
		#endif
		if( MM_GetPhysAddr( (void*)refpage ) )
			gaiPageReferences[page] = 1;
	}
	else
	{
		// Reference again
		if( !MM_GetPhysAddr( (void*)refpage ) )
		{
			 int	pages_per_page, basepage, i;
			if( MM_Allocate( (void*) refpage) == 0 ) {
				// Out of memory, can this be resolved?
				// TODO: Reclaim memory
				Log_Error("PMM", "Out of memory (MM_RefPhys)");
				return ;
			}
			pages_per_page = PAGE_SIZE/sizeof(*gaiPageReferences);
			basepage = page & ~(pages_per_page-1);
			for( i = 0; i < pages_per_page; i ++ ) {
				if( PAGE_BITMAP_FREE(basepage+i) )
					gaiPageReferences[basepage+i] = 0;
				else
					gaiPageReferences[basepage+i] = 1;
			}
			gaiPageReferences[page] = 2;
		}
		else
			gaiPageReferences[ page ] ++;
	}
}

int MM_GetRefCount(tPAddr PAddr)
{
	PAddr >>= 12;
	if( MM_GetPhysAddr( &gaiPageReferences[PAddr] ) ) {
		return gaiPageReferences[PAddr];
	}
	
	Uint32	*bm = &gaPageBitmaps[ PAddr / 32 ];
	if( !MM_GetPhysAddr(bm) ) {
		Log_Error("MMPhys", "MM_GetRefCount: bitmap for ppage 0x%x not mapped %p",
			PAddr, bm);
		return 0;
	}
	if( (*bm) & (1LL << (PAddr&31)) ) {
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

	ENTER("PPAddr", PAddr);
	
	if( MM_GetPhysAddr( &gaiPageReferences[page] ) )
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
	if( MM_GetPhysAddr( &gapPageNodes[page] ) ) {
		gapPageNodes[page] = NULL;
		// TODO: Catch when all pages in this range are not using nodes
	}
	
	// Update the free counts if the page was freed
	if( gaPageBitmaps[ page / 32 ] & (1LL << (page&31)) )
	{
		giPhysNumFree ++;
		if( giPhysFirstFree == -1 || giPhysFirstFree > page )
			giPhysFirstFree = page;
		if( giPhysLastFree < page )
			giPhysLastFree = page;
	}

	#if USE_SUPER_BITMAP	
	// If the bitmap entry is not zero, set the bit free in the super bitmap
	if(gaPageBitmaps[ page / 32 ] != 0 ) {
		gaSuperBitmap[page / (32*32)] |= 1LL << ((page / 32) & 31);
	}
	#endif
	LEAVE('-');
}

int MM_SetPageNode(tPAddr PAddr, void *Node)
{
	tPAddr	page = PAddr >> 12;
	tVAddr	node_page = ((tVAddr)&gapPageNodes[page]) & ~(PAGE_SIZE-1);

	if( !MM_GetRefCount(PAddr) )	return 1;
	
	if( !MM_GetPhysAddr( (void*)node_page ) ) {
		if( !MM_Allocate( (void*)node_page) )
			return -1;
		memset( (void*)node_page, 0, PAGE_SIZE );
	}

	gapPageNodes[page] = Node;
	return 0;
}

int MM_GetPageNode(tPAddr PAddr, void **Node)
{
	if( !MM_GetRefCount(PAddr) )	return 1;
	PAddr >>= 12;
	
	if( !MM_GetPhysAddr( &gapPageNodes[PAddr] ) ) {
		*Node = NULL;
		return 0;
	}

	*Node = gapPageNodes[PAddr];
	return 0;
}

