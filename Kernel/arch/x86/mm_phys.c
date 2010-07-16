/*
 * Acess2
 * - Physical memory manager
 */
#define DEBUG	0
#include <acess.h>
#include <mboot.h>
#include <mm_virt.h>

#define USE_STACK	1

#define	REFERENCE_BASE	0xE0400000

// === IMPORTS ===
extern void	gKernelEnd;

// === PROTOTYPES ===
tPAddr	MM_AllocPhys(void);
tPAddr	MM_AllocPhysRange(int Pages, int MaxBits);
void	MM_RefPhys(tPAddr PAddr);
void	MM_DerefPhys(tPAddr PAddr);

// === GLOBALS ===
Uint64	giPhysAlloc = 0;	// Number of allocated pages
Uint64	giPageCount = 0;	// Total number of pages
Uint64	giLastPossibleFree = 0;	// Last possible free page (before all pages are used)

Uint32	gaSuperBitmap[1024];	// Blocks of 1024 Pages
Uint32	gaPageBitmap[1024*1024/32];	// Individual pages
Uint32	*gaPageReferences;

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
		if(ent->Type == 1 && ent->Base + ent->Length > maxAddr)
			maxAddr = ent->Base + ent->Length;
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
	
	// Allocate References
	//LOG("Reference Pages %i", (giPageCount*4+0xFFF)>>12);
	for(num = 0; num < (giPageCount*4+0xFFF)>>12; num++)
	{
		MM_Allocate( REFERENCE_BASE + (num<<12) );
	}
	
	//LOG("Filling");
	// Fill references
	gaPageReferences = (void*)REFERENCE_BASE;
	memsetd(gaPageReferences, 1, kernelPages);
	for( num = kernelPages; num < giPageCount; num++ )
	{
		gaPageReferences[num] = (gaPageBitmap[ num / 32 ] >> (num&31)) & 1;
	}
}

/**
 * \fn tPAddr MM_AllocPhys(void)
 * \brief Allocates a physical page from the general pool
 */
tPAddr MM_AllocPhys(void)
{
	// int	a, b, c;
	 int	indx;
	tPAddr	ret;
	
	ENTER("");
	
	LOCK( &giPhysAlloc );
	
	// Find free page
	// Scan downwards
	#if 1
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
	LOG("indx = %i", indx);
	#else
	c = giLastPossibleFree % 32;
	b = (giLastPossibleFree / 32) % 32;
	a = giLastPossibleFree / 1024;
	
	LOG("a=%i,b=%i,c=%i", a, b, c);
	for( ; gaSuperBitmap[a] == -1 && a >= 0; a-- );
	if(a < 0) {
		RELEASE( &giPhysAlloc );
		Warning("MM_AllocPhys - OUT OF MEMORY (Called by %p)", __builtin_return_address(0));
		LEAVE('i', 0);
		return 0;
	}
	for( ; gaSuperBitmap[a] & (1<<b); b-- );
	for( ; gaPageBitmap[a*32+b] & (1<<c); c-- );
	LOG("a=%i,b=%i,c=%i", a, b, c);
	indx = (a << 10) | (b << 5) | c;
	#endif
	
	// Mark page used
	if(gaPageReferences)
		gaPageReferences[ indx ] = 1;
	gaPageBitmap[ indx>>5 ] |= 1 << (indx&31);
	
	
	// Get address
	ret = indx << 12;
	giLastPossibleFree = indx;
	
	// Mark used block
	if(gaPageBitmap[ indx>>5 ] == -1)
		gaSuperBitmap[indx>>10] |= 1 << ((indx>>5)&31);

	// Release Spinlock
	RELEASE( &giPhysAlloc );
	
	LEAVE('X', ret);
	//Log("MM_AllocPhys: RETURN 0x%x", ret);
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
	LOCK( &giPhysAlloc );
	
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
		RELEASE( &giPhysAlloc );
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
		RELEASE( &giPhysAlloc );
		Warning("MM_AllocPhysRange - OUT OF MEMORY (Called by %p)", __builtin_return_address(0));
		LEAVE('i', 0);
		return 0;
	}
	
	// Mark pages used
	for( i = 0; i < Pages; i++ )
	{
		if(gaPageReferences)
			gaPageReferences[idx*32+sidx] = 1;
		gaPageBitmap[ idx ] |= 1 << sidx;
		sidx ++;
		if(sidx == 32) { sidx = 0;	idx ++;	}
	}
	
	// Get address
	ret = (idx << 17) | (sidx << 12);
	
	// Mark used block
	if(gaPageBitmap[ idx ] == -1)	gaSuperBitmap[idx/32] |= 1 << (idx%32);

	// Release Spinlock
	RELEASE( &giPhysAlloc );
	
	LEAVE('X', ret);
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
	LOCK( &giPhysAlloc );
	
	// Reference the page
	if(gaPageReferences)
		gaPageReferences[ PAddr ] ++;
	
	// Mark as used
	gaPageBitmap[ PAddr / 32 ] |= 1 << (PAddr&31);
	
	// Mark used block
	if(gaPageBitmap[ PAddr / 32 ] == -1)
		gaSuperBitmap[PAddr/1024] |= 1 << ((PAddr/32)&31);
	
	// Release Spinlock
	RELEASE( &giPhysAlloc );
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
	if(gaPageReferences[ PAddr ] == 0) {
		Warning("MM_DerefPhys - Non-referenced memory dereferenced");
		return;
	}
	
	// Lock Structures
	LOCK( &giPhysAlloc );
	
	if( giLastPossibleFree < PAddr )
		giLastPossibleFree = PAddr;

	// Dereference
	gaPageReferences[ PAddr ] --;
	
	// Mark as free in bitmaps
	if( gaPageReferences[ PAddr ] == 0 )
	{
		//LOG("Freed 0x%x by %p\n", PAddr<<12, __builtin_return_address(0));
		gaPageBitmap[ PAddr / 32 ] &= ~(1 << (PAddr&31));
		if(gaPageReferences[ PAddr ] == 0)
			gaSuperBitmap[ PAddr >> 10 ] &= ~(1 << ((PAddr >> 5)&31));
	}
	
	// Release spinlock
	RELEASE( &giPhysAlloc );
}

/**
 * \fn int MM_GetRefCount(tPAddr Addr)
 */
int MM_GetRefCount(tPAddr Addr)
{
	// Get page number
	Addr >>= 12;
	
	// We don't care about non-ram pages
	if(Addr >= giPageCount)	return -1;
	
	// Check if it is freed
	return gaPageReferences[ Addr ];
}
