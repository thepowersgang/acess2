/*
 AcessOS Microkernel Version
 mm_phys.c
*/
#define DEBUG	1
#include <common.h>
#include <mboot.h>
#include <mm_virt.h>

#define	REFERENCE_BASE	0xE0400000

// === IMPORTS ===
extern void	gKernelEnd;

// === PROTOTYPES ===
Uint32	MM_AllocPhys();
void	MM_RefPhys(Uint32 Addr);
void	MM_DerefPhys(Uint32 Addr);

// === GLOBALS ===
 int	giPhysAlloc = 0;
Uint	giPageCount = 0;
Uint32	gaSuperBitmap[1024];	// Blocks of 1024 Pages
Uint32	gaPageBitmap[1024*1024/32];	// Individual pages
Uint32	*gaPageReferences;

// === CODE ===
void MM_Install(tMBoot_Info *MBoot)
{
	Uint	kernelPages, num;
	Uint	i;
	tMBoot_Module	*mods;
	
	// Initialise globals
	giPageCount = (MBoot->HighMem >> 2) + 256;	// HighMem is a kByte value
	LOG("giPageCount = %i", giPageCount);
	
	// Get used page count
	kernelPages = (Uint)&gKernelEnd - KERNEL_BASE;
	kernelPages += 0xFFF;	// Page Align
	kernelPages >>= 12;
	
	// Fill page bitmap
	num = kernelPages/32;
	memsetd(gaPageBitmap, -1, num);
	gaPageBitmap[ num ] = (1 << (kernelPages & 31)) - 1;
	
	// Fill Superpage bitmap
	num = kernelPages/(32*32);
	memsetd(gaSuperBitmap, -1, num);
	gaSuperBitmap[ num ] = (1 << ((kernelPages / 32) & 31)) - 1;
	
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
	LOG("Reference Pages %i", (giPageCount*4+0xFFF)>>12);
	for(num = 0; num < (giPageCount*4+0xFFF)>>12; num++)
	{
		MM_Allocate( REFERENCE_BASE + (num<<12) );
	}
	
	// Fill references
	gaPageReferences = (void*)REFERENCE_BASE;
	memsetd(gaPageReferences, 1, kernelPages);
	for( num = kernelPages; num < giPageCount; num++ )
	{
		gaPageReferences[num] = (gaPageBitmap[ num / 32 ] >> (num&31)) & 1;
	}
}

/**
 * \fn tPAddr MM_AllocPhys()
 * \brief Allocates a physical page
 */
tPAddr MM_AllocPhys()
{
	 int	num = giPageCount / 32 / 32;
	 int	a, b, c;
	Uint32	ret;
	
	LOCK( &giPhysAlloc );
	
	// Find free page
	for(a=0;gaSuperBitmap[a]==-1&&a<num;a++);
	if(a == num) {
		RELEASE( &giPhysAlloc );
		Warning("MM_AllocPhys - OUT OF MEMORY (Called by %p)", __builtin_return_address(0));
		return 0;
	}
	for(b=0;gaSuperBitmap[a]&(1<<b);b++);
	for(c=0;gaPageBitmap[a*32+b]&(1<<c);c++);
	//for(c=0;gaPageReferences[a*32*32+b*32+c]>0;c++);
	
	// Mark page used
	if(gaPageReferences)
		gaPageReferences[a*32*32+b*32+c] = 1;
	gaPageBitmap[ a*32+b ] |= 1 << c;
	
	// Get address
	ret = (a << 22) + (b << 17) + (c << 12);
	
	// Mark used block
	if(gaPageBitmap[ a*32+b ] == -1)	gaSuperBitmap[a] |= 1 << b;

	// Release Spinlock
	RELEASE( &giPhysAlloc );
	LOG("Allocated 0x%x\n", ret);
	//LOG("ret = %x", ret);
	return ret;
}

/**
 * \fn void MM_RefPhys(tPAddr Addr)
 */
void MM_RefPhys(tPAddr Addr)
{
	// Get page number
	Addr >>= 12;
	
	// We don't care about non-ram pages
	if(Addr >= giPageCount)	return;
	
	// Lock Structures
	LOCK( &giPhysAlloc );
	
	// Reference the page
	if(gaPageReferences)
		gaPageReferences[ Addr ] ++;
	
	// Mark as used
	gaPageBitmap[ Addr / 32 ] |= 1 << (Addr&31);
	
	// Mark used block
	if(gaPageBitmap[ Addr / 32 ] == -1)	gaSuperBitmap[Addr/1024] |= 1 << ((Addr/32)&31);
	
	// Release Spinlock
	RELEASE( &giPhysAlloc );
}

/**
 * \fn void MM_DerefPhys(Uint32 Addr)
 */
void MM_DerefPhys(tPAddr Addr)
{
	// Get page number
	Addr >>= 12;
	
	// We don't care about non-ram pages
	if(Addr >= giPageCount)	return;
	
	// Check if it is freed
	if(gaPageReferences[ Addr ] == 0) {
		Warning("MM_DerefPhys - Non-referenced memory dereferenced");
		return;
	}
	
	// Lock Structures
	LOCK( &giPhysAlloc );
	
	// Dereference
	gaPageReferences[ Addr ] --;
	
	// Mark as free in bitmaps
	if( gaPageReferences[ Addr ] == 0 )
	{
		LOG("Freed 0x%x\n", Addr);
		gaPageBitmap[ Addr / 32 ] &= ~(1 << (Addr&31));
		if(gaPageReferences[ Addr ] == 0)
			gaSuperBitmap[ Addr >> 10 ] &= ~(1 << ((Addr >> 5)&31));
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
