/*
 * Acess2 Kernel VFS
 * - By John Hodge (thePowersGang)
 *
 * mmap.c
 * - VFS_MMap support
 */
#define DEBUG	0
#include <acess.h>
#include <vfs.h>
#include <vfs_ext.h>
#include <vfs_int.h>

#define MMAP_PAGES_PER_BLOCK	16

// === STRUCTURES ===
typedef struct sVFS_MMapPageBlock	tVFS_MMapPageBlock;
struct sVFS_MMapPageBlock
{
	tVFS_MMapPageBlock	*Next;
	Uint64	BaseOffset;	// Must be a multiple of MMAP_PAGES_PER_BLOCK*PAGE_SIZE
	tPAddr	PhysAddrs[MMAP_PAGES_PER_BLOCK];
};

// === PROTOTYPES ===
//void	*VFS_MMap(void *DestHint, size_t Length, int Protection, int Flags, int FD, Uint64 Offset);
void	*VFS_MMap_Anon(void *Destination, size_t Length, Uint FlagsSet, Uint FlagsMask);

// === CODE ===
void *VFS_MMap(void *DestHint, size_t Length, int Protection, int Flags, int FD, Uint64 Offset)
{
	tVAddr	mapping_base;
	 int	npages, pagenum;
	tVFS_MMapPageBlock	*pb, *prev;

	ENTER("pDestHint iLength xProtection xFlags xFD XOffset", DestHint, Length, Protection, Flags, FD, Offset);

	if( Flags & MMAP_MAP_ANONYMOUS )
		Offset = (tVAddr)DestHint & 0xFFF;
	
	npages = ((Offset & (PAGE_SIZE-1)) + Length + (PAGE_SIZE - 1)) / PAGE_SIZE;
	pagenum = Offset / PAGE_SIZE;

	mapping_base = (tVAddr)DestHint;
	tPage	*mapping_dest = (void*)(mapping_base & ~(PAGE_SIZE-1));

	if( DestHint == NULL )
	{
		// TODO: Locate space for the allocation
		LEAVE('n');
		return NULL;
	}

	// Handle anonymous mappings
	if( Flags & MMAP_MAP_ANONYMOUS )
	{
		// TODO: Comvert \a Protection into a flag set
		void	*ret = VFS_MMap_Anon((void*)mapping_base, Length, 0, 0);
		LEAVE_RET('p', ret);
	}

	tVFS_Handle *h = VFS_GetHandle(FD);
	if( !h || !h->Node )	LEAVE_RET('n', NULL);

	LOG("h = %p", h);
	
	Mutex_Acquire( &h->Node->Lock );

	// Search for existing mapping for each page
	// - Sorted list of 16 page blocks
	for(
		pb = h->Node->MMapInfo, prev = NULL;
		pb && pb->BaseOffset + MMAP_PAGES_PER_BLOCK < pagenum;
		prev = pb, pb = pb->Next
		)
		;

	LOG("pb = %p, pb->BaseOffset = %X", pb, pb ? pb->BaseOffset : 0);

	// - Allocate a block if needed
	if( !pb || pb->BaseOffset > pagenum )
	{
		void	*old_pb = pb;
		pb = calloc( 1, sizeof(tVFS_MMapPageBlock) );
		if(!pb) {
			Mutex_Release( &h->Node->Lock );
			LEAVE_RET('n', NULL);
		}
		pb->Next = old_pb;
		pb->BaseOffset = pagenum - pagenum % MMAP_PAGES_PER_BLOCK;
		if(prev)
			prev->Next = pb;
		else
			h->Node->MMapInfo = pb;
	}

	// - Map (and allocate) pages
	while( npages -- )
	{
		if( MM_GetPhysAddr( mapping_dest ) == 0 )
		{
			if( pb->PhysAddrs[pagenum - pb->BaseOffset] == 0 )
			{
				tVFS_NodeType	*nt = h->Node->Type;
				if( !nt ) 
				{
					// TODO: error
				}
				else if( nt->MMap )
					nt->MMap(h->Node, pagenum*PAGE_SIZE, PAGE_SIZE, mapping_dest);
				else
				{
					 int	read_len;
					// Allocate pages and read data
					if( MM_Allocate(mapping_dest) == 0 ) {
						// TODO: Unwrap
						Mutex_Release( &h->Node->Lock );
						LEAVE('n');
						return NULL;
					}
					// TODO: Clip read length
					read_len = nt->Read(h->Node, pagenum*PAGE_SIZE, PAGE_SIZE,
						mapping_dest, 0);
					// TODO: This was commented out, why?
					if( read_len != PAGE_SIZE ) {
						memset( (char*)mapping_dest + read_len, 0, PAGE_SIZE-read_len );
					}
				}
				pb->PhysAddrs[pagenum - pb->BaseOffset] = MM_GetPhysAddr( mapping_dest );
				MM_SetPageNode( pb->PhysAddrs[pagenum - pb->BaseOffset], h->Node );
				MM_RefPhys( pb->PhysAddrs[pagenum - pb->BaseOffset] );
				LOG("Read and map %X to %p (%P)", pagenum*PAGE_SIZE, mapping_dest,
					pb->PhysAddrs[pagenum - pb->BaseOffset]);
			}
			else
			{
				MM_Map( mapping_dest, pb->PhysAddrs[pagenum - pb->BaseOffset] );
				MM_RefPhys( pb->PhysAddrs[pagenum - pb->BaseOffset] );
				LOG("Cached map %X to %p (%P)", pagenum*PAGE_SIZE, mapping_dest,
					pb->PhysAddrs[pagenum - pb->BaseOffset]);
			}
			h->Node->ReferenceCount ++;
		
			// Set flags
			if( !(Protection & MMAP_PROT_WRITE) ) {
				MM_SetFlags(mapping_dest, MM_PFLAG_RO, MM_PFLAG_RO);
			}
			else {
				MM_SetFlags(mapping_dest, 0, MM_PFLAG_RO);
			}
			
			if( Protection & MMAP_PROT_EXEC ) {
				MM_SetFlags(mapping_dest, MM_PFLAG_EXEC, MM_PFLAG_EXEC);
			}
			else {
				MM_SetFlags(mapping_dest, 0, MM_PFLAG_EXEC);
			}
		}
		else
		{
			LOG("Flag update on %p", mapping_dest);
			if( (MM_GetFlags(mapping_dest) & MM_PFLAG_RO) && (Protection & MMAP_PROT_WRITE) )
			{
				MM_SetFlags(mapping_dest, 0, MM_PFLAG_RO);
			}
		}
		if( Flags & MMAP_MAP_PRIVATE )
			MM_SetFlags(mapping_dest, MM_PFLAG_COW, MM_PFLAG_COW);
		pagenum ++;
		mapping_dest ++;

		// Roll on to next block if needed
		if(pagenum - pb->BaseOffset == MMAP_PAGES_PER_BLOCK)
		{
			if( pb->Next && pb->Next->BaseOffset == pagenum )
				pb = pb->Next;
			else
			{
				tVFS_MMapPageBlock	*oldpb = pb;
				pb = malloc( sizeof(tVFS_MMapPageBlock) );
				pb->Next = oldpb->Next;
				pb->BaseOffset = pagenum;
				memset(pb->PhysAddrs, 0, sizeof(pb->PhysAddrs));
				oldpb->Next = pb;
			}
		}
	}
	
	Mutex_Release( &h->Node->Lock );

	LEAVE('p', mapping_base);
	return (void*)mapping_base;
}

void *VFS_MMap_Anon(void *Destination, size_t Length, Uint FlagsSet, Uint FlagsMask)
{
	size_t	ofs = (tVAddr)Destination & (PAGE_SIZE-1);
	tPage	*mapping_dest = (void*)( (char*)Destination - ofs );
	
	if( ofs > 0 )
	{
		size_t	bytes = MIN(PAGE_SIZE - ofs, Length);
		
		// Allocate a partial page
		if( MM_GetPhysAddr(mapping_dest) )
		{
			// Already allocated page, clear the area we're touching
			ASSERT( ofs + bytes <= PAGE_SIZE );
			
			// TODO: Double check that this area isn't already zero
			memset( Destination, 0, bytes );
			
			MM_SetFlags(mapping_dest, FlagsSet, FlagsMask);
			
			LOG("#1: Clear %i from %p", Length, Destination);
		}
		else
		{
			MM_AllocateZero(mapping_dest);
			LOG("#1: Allocate for %p", Destination);
		}
		mapping_dest ++;
		Length -= bytes;
	}
	while( Length >= PAGE_SIZE )
	{
		if( MM_GetPhysAddr( mapping_dest ) )
		{
			// We're allocating entire pages here, so free this page and replace with a COW zero
			MM_Deallocate(mapping_dest);
			LOG("Replace %p with zero page", mapping_dest);
		}
		else
		{
			LOG("Allocate zero at %p", mapping_dest);
		}
		MM_AllocateZero(mapping_dest);
		
		mapping_dest ++;
		Length -= PAGE_SIZE;
	}
	if( Length > 0 )
	{
		ASSERT(Length < PAGE_SIZE);
		
		// Tail page
		if( MM_GetPhysAddr(mapping_dest) )
		{
			// TODO: Don't touch page if already zero
			memset( mapping_dest, 0, Length );
			LOG("Clear %i in %p", Length, mapping_dest);
		}
		else
		{
			MM_AllocateZero(mapping_dest);
			LOG("Anon map to %p", mapping_dest);
		}
	}
	
	return Destination;
}

int VFS_MUnmap(void *Addr, size_t Length)
{
	return 0;
}
