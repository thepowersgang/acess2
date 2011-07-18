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
#define PAGE_SIZE	0x1000	// Should be in mm_virt.h

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
	tVAddr	mapping_dest;
	 int	npages, pagenum;
	tVFS_MMapPageBlock	*pb, *prev;
	
	npages = ((Offset & (PAGE_SIZE-1)) + Length) / PAGE_SIZE;
	pagenum = Offset / PAGE_SIZE;

	mapping_dest = (tVAddr)DestHint;	

#if 0
	// TODO: Locate space for the allocation
	if( Flags & MAP_ANONYMOUS )
	{
		MM_Allocate(mapping_dest);
		return (void*)mapping_dest;
	}
#endif

	h = VFS_GetHandle(FD);
	if( !h || !h->Node )	return NULL;

	// Search for existing mapping for each page
	// - Sorted list of 16 page blocks
	for(
		pb = h->Node->MMapInfo, prev = NULL;
		pb && pb->BaseOffset + MMAP_PAGES_PER_BLOCK < pagenum;
		prev = pb, pb = pb->Next
		);

	// - Allocate a block if needed
	if( !pb || pb->BaseOffset > pagenum )
	{
		void	*old_pb = pb;
		pb = malloc( sizeof(tVFS_MMapPageBlock) );
		if(!pb)	return NULL;
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
		if( pb->PhysAddrs[pagenum - pb->BaseOffset] == 0 )
		{
			if( h->Node->MMap )
				h->Node->MMap(h->Node, pagenum*PAGE_SIZE, PAGE_SIZE, (void*)mapping_dest);
			else
			{
				// Allocate pages and read data
				if( MM_Allocate(mapping_dest) == 0 ) {
					// TODO: Unwrap
					return NULL;
				}
				h->Node->Read(h->Node, pagenum*PAGE_SIZE, PAGE_SIZE, (void*)mapping_dest);
			}
			pb->PhysAddrs[pagenum - pb->BaseOffset] = MM_GetPhysAddr( mapping_dest );
//			MM_SetPageInfo( pb->PhysAddrs[pagenum - pb->BaseOffset], h->Node, pagenum*PAGE_SIZE );
		}
		else
		{
			MM_Map( mapping_dest, pb->PhysAddrs[pagenum - pb->BaseOffset] );
		}
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
	
	return NULL;
}

int VFS_MUnmap(void *Addr, size_t Length)
{
	return 0;
}
