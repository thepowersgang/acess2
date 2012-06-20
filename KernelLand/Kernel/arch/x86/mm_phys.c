/*
 * Acess2
 * - Physical memory manager
 */
#define DEBUG	0
#include <acess.h>
#include <mboot.h>
#include <mm_virt.h>

//#define USE_STACK	1
#define TRACE_ALLOCS	0	// Print trace messages on AllocPhys/DerefPhys

static const int addrClasses[] = {0,16,20,24,32,64};
static const int numAddrClasses = sizeof(addrClasses)/sizeof(addrClasses[0]);

// === IMPORTS ===
extern char	gKernelEnd[];
extern void	Proc_PrintBacktrace(void);

// === PROTOTYPES ===
void	MM_Install(tMBoot_Info *MBoot);
//tPAddr	MM_AllocPhys(void);
//tPAddr	MM_AllocPhysRange(int Pages, int MaxBits);
//void	MM_RefPhys(tPAddr PAddr);
//void	MM_DerefPhys(tPAddr PAddr);
// int	MM_GetRefCount(tPAddr PAddr);

// === GLOBALS ===
tMutex	glPhysAlloc;
Uint64	giPhysAlloc = 0;	// Number of allocated pages
Uint64	giPageCount = 0;	// Total number of pages
Uint64	giLastPossibleFree = 0;	// Last possible free page (before all pages are used)
Uint64	giTotalMemorySize = 0;	// Total number of allocatable pages

Uint32	gaSuperBitmap[1024];	// Blocks of 1024 Pages
Uint32	gaPageBitmap[1024*1024/32];	// Individual pages
 int	*gaPageReferences;
void	**gaPageNodes = (void*)MM_PAGENODE_BASE;
#define REFENT_PER_PAGE	(0x1000/sizeof(gaPageReferences[0]))

// === CODE ===
void MM_Install(tMBoot_Info *MBoot)
{
	Uint	kernelPages, num;
	Uint	i;
	Uint64	maxAddr = 0;
	tMBoot_Module	*mods;
	tMBoot_MMapEnt	*ent;
	
	// --- Find largest address
	MBoot->MMapAddr |= KERNEL_BASE;
	ent = (void *)( MBoot->MMapAddr );
	while( (Uint)ent < MBoot->MMapAddr + MBoot->MMapLength )
	{
		// Adjust for size
		ent->Size += 4;
		
		// If entry is RAM and is above `maxAddr`, change `maxAddr`
		if(ent->Type == 1)
		{
			if(ent->Base + ent->Length > maxAddr)
				maxAddr = ent->Base + ent->Length;
			giTotalMemorySize += ent->Length >> 12;
		}
		// Go to next entry
		ent = (tMBoot_MMapEnt *)( (Uint)ent + ent->Size );
	}
	
	if(maxAddr == 0) {	
		giPageCount = (MBoot->HighMem >> 2) + 256;	// HighMem is a kByte value
	}
	else {
		giPageCount = maxAddr >> 12;
	}
	giLastPossibleFree = giPageCount - 1;
	
	memsetd(gaPageBitmap, 0xFFFFFFFF, giPageCount/32);
	
	// Set up allocateable space
	ent = (void *)( MBoot->MMapAddr );
	while( (Uint)ent < MBoot->MMapAddr + MBoot->MMapLength )
	{		
		memsetd( &gaPageBitmap[ent->Base/(4096*32)], 0, ent->Length/(4096*32) );
		ent = (tMBoot_MMapEnt *)( (Uint)ent + ent->Size );
	}
	
	// Get used page count (Kernel)
	kernelPages = (Uint)&gKernelEnd - KERNEL_BASE - 0x100000;
	kernelPages += 0xFFF;	// Page Align
	kernelPages >>= 12;
	giPhysAlloc += kernelPages;	// Add to used count	

	// Fill page bitmap
	num = kernelPages/32;
	memsetd( &gaPageBitmap[0x100000/(4096*32)], -1, num );
	gaPageBitmap[ 0x100000/(4096*32) + num ] = (1 << (kernelPages & 31)) - 1;
	
	// Fill Superpage bitmap
	num = kernelPages/(32*32);
	memsetd( &gaSuperBitmap[0x100000/(4096*32*32)], -1, num );
	gaSuperBitmap[ 0x100000/(4096*32*32) + num ] = (1 << ((kernelPages / 32) & 31)) - 1;
	
	// Mark Multiboot's pages as taken
	// - Structure
	MM_RefPhys( (Uint)MBoot - KERNEL_BASE );
	// - Module List
	for(i = (MBoot->ModuleCount*sizeof(tMBoot_Module)+0xFFF)>12; i--; )
		MM_RefPhys( MBoot->Modules + (i << 12) );
	// - Modules
	mods = (void*)(MBoot->Modules + KERNEL_BASE);
	for(i = 0; i < MBoot->ModuleCount; i++)
	{
		num = (mods[i].End - mods[i].Start + 0xFFF) >> 12;
		while(num--)
			MM_RefPhys( (mods[i].Start & ~0xFFF) + (num<<12) );
	}

	gaPageReferences = (void*)MM_REFCOUNT_BASE;

	Log_Log("PMem", "Physical memory set up (%lli pages of ~%lli MiB used)",
		giPhysAlloc, (giTotalMemorySize*4)/1024
		);
}

void MM_DumpStatistics(void)
{
	 int	i, pg;
	for( i = 1; i < numAddrClasses; i ++ )
	{
		 int	first = (i == 1 ? 0 : (1UL << (addrClasses[i-1] - 12)));
		 int	last  = (1UL << (addrClasses[i] - 12)) - 1;
		 int	nFree = 0;
		 int	nMultiRef = 0;
		 int	totalRefs = 0;

		if( last > giPageCount )
			last = giPageCount;
		
		 int	total = last - first + 1;
	
		for( pg = first; pg < last; pg ++ )
		{
			if( !MM_GetPhysAddr(&gaPageReferences[pg]) || gaPageReferences[pg] == 0 ) {
				nFree ++;
				continue ;
			}
			totalRefs += gaPageReferences[pg];
			if(gaPageReferences[pg] > 1)
				nMultiRef ++;
		}
		
		 int	nUsed = (total - nFree);
		Log_Log("MMPhys", "%ipbit - %i/%i used, %i reused, %i average reference count",
			addrClasses[i], nUsed, total, nMultiRef,
			nMultiRef ? (totalRefs-(nUsed - nMultiRef)) / nMultiRef : 0
			);
		
		if( last == giPageCount )
			break;
	}
	Log_Log("MMPhys", "%lli/%lli total pages used, 0 - %i possible free range",
		giPhysAlloc, giTotalMemorySize, giLastPossibleFree);
}

/**
 * \fn tPAddr MM_AllocPhys(void)
 * \brief Allocates a physical page from the general pool
 */
tPAddr MM_AllocPhys(void)
{
	// int	a, b, c;
	 int	indx = -1;
	tPAddr	ret;
	
	ENTER("");
	
	Mutex_Acquire( &glPhysAlloc );
	
	// Classful scan
	#if 1
	{
	 int	i;
	 int	first, last;
	for( i = numAddrClasses; i -- > 1; )
	{
		first = 1UL << (addrClasses[i-1] - 12);
		last = (1UL << (addrClasses[i] - 12)) - 1;
		// Range is above the last free page
		if( first > giLastPossibleFree )
			continue;
		// Last possible free page is in the range
		if( last > giLastPossibleFree )
			last = giLastPossibleFree;
			
		// Scan the range
		for( indx = first; indx < last; )
		{
			if( gaSuperBitmap[indx>>10] == -1 ) {
				indx += 1024;
				continue;
			}
			
			if( gaPageBitmap[indx>>5] == -1 ) {
				indx += 32;
				continue;
			}
			
			if( gaPageBitmap[indx>>5] & (1 << (indx&31)) ) {
				indx ++;
				continue;
			}
			break;
		}
		if( indx < last )	break;
		
		giLastPossibleFree = first;	// Well, we couldn't find any in this range
	}
	// Out of memory?
	if( i <= 1 )	indx = -1;
	}
	#elif 0
	// Find free page
	// Scan downwards
	LOG("giLastPossibleFree = %i", giLastPossibleFree);
	for( indx = giLastPossibleFree; indx >= 0; )
	{
		if( gaSuperBitmap[indx>>10] == -1 ) {
			indx -= 1024;
			continue;
		}
		
		if( gaPageBitmap[indx>>5] == -1 ) {
			indx -= 32;
			continue;
		}
		
		if( gaPageBitmap[indx>>5] & (1 << (indx&31)) ) {
			indx --;
			continue;
		}
		break;
	}
	if( indx >= 0 )
		giLastPossibleFree = indx;
	LOG("indx = %i", indx);
	#else
	c = giLastPossibleFree % 32;
	b = (giLastPossibleFree / 32) % 32;
	a = giLastPossibleFree / 1024;
	
	LOG("a=%i,b=%i,c=%i", a, b, c);
	for( ; gaSuperBitmap[a] == -1 && a >= 0; a-- );
	if(a < 0) {
		Mutex_Release( &glPhysAlloc );
		Warning("MM_AllocPhys - OUT OF MEMORY (Called by %p) - %lli/%lli used",
			__builtin_return_address(0), giPhysAlloc, giPageCount);
		LEAVE('i', 0);
		return 0;
	}
	for( ; gaSuperBitmap[a] & (1<<b); b-- );
	for( ; gaPageBitmap[a*32+b] & (1<<c); c-- );
	LOG("a=%i,b=%i,c=%i", a, b, c);
	indx = (a << 10) | (b << 5) | c;
	if( indx >= 0 )
		giLastPossibleFree = indx;
	#endif
	
	if( indx < 0 ) {
		Mutex_Release( &glPhysAlloc );
		Warning("MM_AllocPhys - OUT OF MEMORY (Called by %p) - %lli/%lli used (indx = %x)",
			__builtin_return_address(0), giPhysAlloc, giPageCount, indx);
		Log_Debug("PMem", "giLastPossibleFree = %lli", giLastPossibleFree);
		LEAVE('i', 0);
		return 0;
	}
	
	if( indx > 0xFFFFF ) {
		Panic("The fuck? Too many pages! (indx = 0x%x)", indx);
	}
	
	if( indx >= giPageCount ) {
		Mutex_Release( &glPhysAlloc );
		Log_Error("PMem", "MM_AllocPhys - indx(%i) > giPageCount(%i)", indx, giPageCount);
		LEAVE('i', 0);
		return 0;
	}
	
	// Mark page used
	if( MM_GetPhysAddr( (tVAddr)&gaPageReferences[indx] ) )
		gaPageReferences[indx] = 1;
	gaPageBitmap[ indx>>5 ] |= 1 << (indx&31);
	
	giPhysAlloc ++;
	
	// Get address
	ret = indx << 12;
	
	// Mark used block
	if(gaPageBitmap[ indx>>5 ] == -1) {
		gaSuperBitmap[indx>>10] |= 1 << ((indx>>5)&31);
	}

	// Release Spinlock
	Mutex_Release( &glPhysAlloc );
	
	LEAVE('X', ret);
	#if TRACE_ALLOCS
	if( now() > 4000 ) {
	Log_Debug("PMem", "MM_AllocPhys: RETURN %P (%i free)", ret, giPageCount-giPhysAlloc);
	Proc_PrintBacktrace();
	}
	#endif
	return ret;
}

/**
 * \fn tPAddr MM_AllocPhysRange(int Pages, int MaxBits)
 * \brief Allocate a range of physical pages
 * \param Pages	Number of pages to allocate
 * \param MaxBits	Maximum number of address bits to use
 */
tPAddr MM_AllocPhysRange(int Pages, int MaxBits)
{
	 int	a, b;
	 int	i, idx, sidx;
	tPAddr	ret;
	
	ENTER("iPages iMaxBits", Pages, MaxBits);
	
	// Sanity Checks
	if(MaxBits < 0) {
		LEAVE('i', 0);
		return 0;
	}
	if(MaxBits > PHYS_BITS)	MaxBits = PHYS_BITS;
	
	// Lock
	Mutex_Acquire( &glPhysAlloc );
	
	// Set up search state
	if( giLastPossibleFree > ((tPAddr)1 << (MaxBits-12)) ) {
		sidx = (tPAddr)1 << (MaxBits-12);
	}
	else {
		sidx = giLastPossibleFree;
	}
	idx = sidx / 32;
	sidx %= 32;
	b = idx % 32;
	a = idx / 32;
	
	#if 0
	LOG("a=%i, b=%i, idx=%i, sidx=%i", a, b, idx, sidx);
	
	// Find free page
	for( ; gaSuperBitmap[a] == -1 && a --; )	b = 31;
	if(a < 0) {
		Mutex_Release( &glPhysAlloc );
		Warning("MM_AllocPhysRange - OUT OF MEMORY (Called by %p)", __builtin_return_address(0));
		LEAVE('i', 0);
		return 0;
	}
	LOG("a = %i", a);
	for( ; gaSuperBitmap[a] & (1 << b); b-- )	sidx = 31;
	LOG("b = %i", b);
	idx = a * 32 + b;
	for( ; gaPageBitmap[idx] & (1 << sidx); sidx-- )
		LOG("gaPageBitmap[%i] = 0x%08x", idx, gaPageBitmap[idx]);
	
	LOG("idx = %i, sidx = %i", idx, sidx);
	#else
	
	#endif
	
	// Check if the gap is large enough
	while( idx >= 0 )
	{
		// Find a free page
		for( ; ; )
		{
			// Bulk Skip
			if( gaPageBitmap[idx] == -1 ) {
				idx --;
				sidx = 31;
				continue;
			}
			
			if( gaPageBitmap[idx] & (1 << sidx) ) {
				sidx --;
				if(sidx < 0) {	sidx = 31;	idx --;	}
				if(idx < 0)	break;
				continue;
			}
			break;
		}
		if( idx < 0 )	break;
		
		// Check if it is a free range
		for( i = 0; i < Pages; i++ )
		{
			// Used page? break
			if( gaPageBitmap[idx] & (1 << sidx) )
				break;
			
			sidx --;
			if(sidx < 0) {	sidx = 31;	idx --;	}
			if(idx < 0)	break;
		}
		
		if( i == Pages )
			break;
	}
	
	// Check if an address was found
	if( idx < 0 ) {
		Mutex_Release( &glPhysAlloc );
		Warning("MM_AllocPhysRange - OUT OF MEMORY (Called by %p)", __builtin_return_address(0));
		LEAVE('i', 0);
		return 0;
	}
	
	// Mark pages used
	for( i = 0; i < Pages; i++ )
	{
		if( MM_GetPhysAddr( (tVAddr)&gaPageReferences[idx*32+sidx] ) )
			gaPageReferences[idx*32+sidx] = 1;
		gaPageBitmap[ idx ] |= 1 << sidx;
		sidx ++;
		giPhysAlloc ++;
		if(sidx == 32) { sidx = 0;	idx ++;	}
	}
	
	// Get address
	ret = (idx << 17) | (sidx << 12);
	
	// Mark used block
	if(gaPageBitmap[ idx ] == -1)	gaSuperBitmap[idx/32] |= 1 << (idx%32);

	// Release Spinlock
	Mutex_Release( &glPhysAlloc );
	
	LEAVE('X', ret);
	#if TRACE_ALLOCS
	Log_Debug("PMem", "MM_AllocPhysRange: RETURN 0x%llx-0x%llx (%i free)",
		ret, ret + (1<<Pages)-1, giPageCount-giPhysAlloc);
	#endif
	return ret;
}

/**
 * \fn void MM_RefPhys(tPAddr PAddr)
 */
void MM_RefPhys(tPAddr PAddr)
{
	// Get page number
	PAddr >>= 12;

	// We don't care about non-ram pages
	if(PAddr >= giPageCount)	return;
	
	// Lock Structures
	Mutex_Acquire( &glPhysAlloc );
	
	// Reference the page
	if( gaPageReferences )
	{
		if( MM_GetPhysAddr( (tVAddr)&gaPageReferences[PAddr] ) == 0 )
		{
			 int	i, base;
			tVAddr	addr = ((tVAddr)&gaPageReferences[PAddr]) & ~0xFFF;
//			Log_Debug("PMem", "MM_RefPhys: Allocating info for %X", PAddr);
			Mutex_Release( &glPhysAlloc );
			if( MM_Allocate( addr ) == 0 ) {
				Log_KernelPanic("PMem",
					"MM_RefPhys: Out of physical memory allocating info for %X",
					PAddr*PAGE_SIZE
					);
			}
			Mutex_Acquire( &glPhysAlloc );
			
			base = PAddr & ~(1024-1);
			for( i = 0; i < 1024; i ++ ) {
				gaPageReferences[base + i] = (gaPageBitmap[(base+i)/32] & (1 << (base+i)%32)) ? 1 : 0;
			}
		}
		gaPageReferences[ PAddr ] ++;
	}

	// If not already used
	if( !(gaPageBitmap[ PAddr / 32 ] & 1 << (PAddr&31)) ) {
		giPhysAlloc ++;
		// Mark as used
		gaPageBitmap[ PAddr / 32 ] |= 1 << (PAddr&31);
	}
	
	// Mark used block
	if(gaPageBitmap[ PAddr / 32 ] == -1)
		gaSuperBitmap[PAddr/1024] |= 1 << ((PAddr/32)&31);
	
	// Release Spinlock
	Mutex_Release( &glPhysAlloc );
}

/**
 * \fn void MM_DerefPhys(tPAddr PAddr)
 * \brief Dereferences a physical page
 */
void MM_DerefPhys(tPAddr PAddr)
{
	// Get page number
	PAddr >>= 12;

	// We don't care about non-ram pages
	if(PAddr >= giPageCount)	return;
	
	// Check if it is freed
	if( !(gaPageBitmap[PAddr / 32] & (1 << PAddr%32)) ) {
		Log_Warning("MMVirt", "MM_DerefPhys - Non-referenced memory dereferenced");
		return;
	}
	
	// Lock Structures
	Mutex_Acquire( &glPhysAlloc );
	
	if( giLastPossibleFree < PAddr )
		giLastPossibleFree = PAddr;

	// Dereference
	if( !MM_GetPhysAddr( (tVAddr)&gaPageReferences[PAddr] ) || (-- gaPageReferences[PAddr]) == 0 )
	{
		#if TRACE_ALLOCS
		Log_Debug("PMem", "MM_DerefPhys: Free'd %P (%i free)", PAddr<<12, giPageCount-giPhysAlloc);
		Proc_PrintBacktrace();
		#endif
		//LOG("Freed 0x%x by %p\n", PAddr<<12, __builtin_return_address(0));
		giPhysAlloc --;
		gaPageBitmap[ PAddr / 32 ] &= ~(1 << (PAddr&31));
		if(gaPageBitmap[ PAddr / 32 ] == 0)
			gaSuperBitmap[ PAddr >> 10 ] &= ~(1 << ((PAddr >> 5)&31));

		if( MM_GetPhysAddr( (tVAddr) &gaPageNodes[PAddr] ) )
		{
			gaPageNodes[PAddr] = NULL;
			// TODO: Free Node Page when fully unused
		}
	}

	// Release spinlock
	Mutex_Release( &glPhysAlloc );
}

/**
 * \fn int MM_GetRefCount(tPAddr Addr)
 */
int MM_GetRefCount(tPAddr PAddr)
{
	// Get page number
	PAddr >>= 12;
	
	// We don't care about non-ram pages
	if(PAddr >= giPageCount)	return -1;

	if( MM_GetPhysAddr( (tVAddr)&gaPageReferences[PAddr] ) == 0 )
		return (gaPageBitmap[PAddr / 32] & (1 << PAddr%32)) ? 1 : 0;
	
	// Check if it is freed
	return gaPageReferences[ PAddr ];
}

int MM_SetPageNode(tPAddr PAddr, void *Node)
{
	tVAddr	block_addr;
	
	if( MM_GetRefCount(PAddr) == 0 )	return 1;
	 
	PAddr /= PAGE_SIZE;

	block_addr = (tVAddr) &gaPageNodes[PAddr];
	block_addr &= ~(PAGE_SIZE-1);
	
	if( !MM_GetPhysAddr( block_addr ) )
	{
		if( !MM_Allocate( block_addr ) ) {
			Log_Warning("PMem", "Unable to allocate Node page");
			return -1;
		}
		memset( (void*)block_addr, 0, PAGE_SIZE );
	}

	gaPageNodes[PAddr] = Node;
//	Log("gaPageNodes[0x%x] = %p", PAddr, Node);
	return 0;
}

int MM_GetPageNode(tPAddr PAddr, void **Node)
{
	if( MM_GetRefCount(PAddr) == 0 )	return 1;
	
	PAddr /= PAGE_SIZE;
	if( !MM_GetPhysAddr( (tVAddr) &gaPageNodes[PAddr] ) ) {
		*Node = NULL;
		return 0;
	}
	*Node = gaPageNodes[PAddr];
	return 0;
}

