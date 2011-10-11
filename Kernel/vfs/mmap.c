/*
 * Acess2 VFS
 * - Open, Close and ChDir
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

// === CODE ===
void *VFS_MMap(void *DestHint, size_t Length, int Protection, int Flags, int FD, Uint64 Offset)
{
	tVFS_Handle	*h;
	tVAddr	mapping_dest, mapping_base;
	 int	npages, pagenum;
	tVFS_MMapPageBlock	*pb, *prev;

	ENTER("pDestHint iLength xProtection xFlags xFD XOffset", DestHint, Length, Protection, Flags, FD, Offset);

	npages = ((Offset & (PAGE_SIZE-1)) + Length + (PAGE_SIZE - 1)) / PAGE_SIZE;
	pagenum = Offset / PAGE_SIZE;

	mapping_base = (tVAddr)DestHint;
	mapping_dest = mapping_base & ~(PAGE_SIZE-1);

	// TODO: Locate space for the allocation

	// Handle anonymous mappings
	if( Flags & MMAP_MAP_ANONYMOUS )
	{
		size_t	ofs = 0;
		for( ; npages --; mapping_dest += PAGE_SIZE, ofs += PAGE_SIZE )
		{
			if( MM_GetPhysAddr(mapping_dest) ) {
				// TODO: Set flags to COW if needed (well, if shared)
				MM_SetFlags(mapping_dest, MM_PFLAG_COW, MM_PFLAG_COW);
				memset( (void*)(mapping_base + ofs), 0, PAGE_SIZE - (mapping_base & (PAGE_SIZE-1)));
			}
			else {
				// TODO: Map a COW zero page instead
				if( !MM_Allocate(mapping_dest) ) {
					// TODO: Error
					Log_Warning("VFS", "VFS_MMap: Anon alloc to %p failed", mapping_dest);
				}
				memset((void*)mapping_dest, 0, PAGE_SIZE);
				LOG("Anon map to %p", mapping_dest);
			}
		}
		LEAVE_RET('p', (void*)mapping_base);
	}

	h = VFS_GetHandle(FD);
	if( !h || !h->Node )	LEAVE_RET('n', NULL);

	LOG("h = %p", h);
	
	Mutex_Acquire( &h->Node->Lock );

	// Search for existing mapping for each page
	// - Sorted list of 16 page blocks
	for(
		pb = h->Node->MMapInfo, prev = NULL;
		pb && pb->BaseOffset + MMAP_PAGES_PER_BLOCK < pagenum;
		prev = pb, pb = pb->Next
		);

	LOG("pb = %p, pb->BaseOffset = %X", pb, pb ? pb->BaseOffset : 0);

	// - Allocate a block if needed
	if( !pb || pb->BaseOffset > pagenum )
	{
		void	*old_pb = pb;
		pb = malloc( sizeof(tVFS_MMapPageBlock) );
		if(!pb) {
			Mutex_Release( &h->Node->Lock );
			LEAVE_RET('n', NULL);
		}
		pb->Next = old_pb;
		pb->BaseOffset = pagenum - pagenum % MMAP_PAGES_PER_BLOCK;
		memset(pb->PhysAddrs, 0, sizeof(pb->PhysAddrs));
		if(prev)
			prev->Next = pb;
		else
			h->Node->MMapInfo = pb;
	}

	// - Map (and allocate) pages
	while( npages -- )
	{
		if( MM_GetPhysAddr(mapping_dest) == 0 )
		{
			if( pb->PhysAddrs[pagenum - pb->BaseOffset] == 0 )
			{
				if( h->Node->MMap )
					h->Node->MMap(h->Node, pagenum*PAGE_SIZE, PAGE_SIZE, (void*)mapping_dest);
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
					read_len = h->Node->Read(h->Node, pagenum*PAGE_SIZE, PAGE_SIZE, (void*)mapping_dest);
//					if( read_len != PAGE_SIZE ) {
//						memset( (void*)(mapping_dest+read_len), 0, PAGE_SIZE-read_len );
//					}
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
		mapping_dest += PAGE_SIZE;

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
			pagenum = 0;
		}
	}
	
	Mutex_Release( &h->Node->Lock );

	LEAVE('p', mapping_base);
	return (void*)mapping_base;
}

int VFS_MUnmap(void *Addr, size_t Length)
{
	return 0;
}
