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
int	VFS_MMap_MapPage(tVFS_Node *Node, unsigned int PageNum, tVFS_MMapPageBlock *pb, void *mapping_dest, unsigned int Protection);
//int	VFS_MUnmap(void *Addr, size_t Length);

// === CODE ===
void *VFS_MMap(void *DestHint, size_t Length, int Protection, int Flags, int FD, Uint64 Offset)
{
	ENTER("pDestHint iLength xProtection xFlags xFD XOffset", DestHint, Length, Protection, Flags, FD, Offset);

	if( Flags & MMAP_MAP_ANONYMOUS )
		Offset = (tVAddr)DestHint & 0xFFF;
	
	unsigned int npages = ((Offset & (PAGE_SIZE-1)) + Length + (PAGE_SIZE - 1)) / PAGE_SIZE;
	unsigned int pagenum = Offset / PAGE_SIZE;

	tVAddr mapping_base = (tVAddr)DestHint;

	if( Flags & MMAP_MAP_FIXED )
	{
		ASSERT( (Flags & MMAP_MAP_FIXED) && DestHint != NULL );
		// Keep and use the hint
		// - TODO: Validate that the region pointed to by the hint is correct
	}
	else
	{
		Log_Warning("VFS", "MMap: TODO Handle non-fixed mappings");
		if( DestHint == NULL )
		{
			// TODO: Locate space for the allocation
			Log_Warning("VFS", "Mmap: Handle NULL destination hint");
			LEAVE('n');
			return NULL;
		}
	}
	tPage	*mapping_dest = (void*)(mapping_base & ~(PAGE_SIZE-1));

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

	tVFS_MMapPageBlock	*pb, **pb_pnp = (tVFS_MMapPageBlock**)&h->Node->MMapInfo;
	// Search for existing mapping for each page
	// - Sorted list of 16 page blocks
	for( pb = h->Node->MMapInfo; pb; pb_pnp = &pb->Next, pb = pb->Next )
	{
		if( pb->BaseOffset + MMAP_PAGES_PER_BLOCK <= pagenum )
			break;
	}

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
		*pb_pnp = pb;
	}

	// - Map (and allocate) pages
	while( npages -- )
	{
		ASSERTC( pagenum, >=, pb->BaseOffset );
		ASSERTC( pagenum - pb->BaseOffset, <, MMAP_PAGES_PER_BLOCK );
		if( MM_GetPhysAddr( mapping_dest ) == 0 )
		{
			LOG("Map page to %p", mapping_dest);
			if( VFS_MMap_MapPage(h->Node, pagenum, pb, mapping_dest, Protection) )
			{
				Mutex_Release( &h->Node->Lock );
				LEAVE('n');
				return NULL;
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
		if( Flags & MMAP_MAP_PRIVATE ) {
			// TODO: Don't allow the page to change underneath either
			MM_SetFlags(mapping_dest, MM_PFLAG_COW, MM_PFLAG_COW);
		}
		pagenum ++;
		mapping_dest ++;

		// Roll on to next block if needed
		if(pagenum - pb->BaseOffset == MMAP_PAGES_PER_BLOCK)
		{
			if( !pb->Next || pb->Next->BaseOffset != pagenum )
			{
				if( pb->Next )	ASSERTC(pb->Next->BaseOffset % MMAP_PAGES_PER_BLOCK, ==, 0);
				tVFS_MMapPageBlock	*newpb = malloc( sizeof(tVFS_MMapPageBlock) );
				newpb->Next = pb->Next;
				newpb->BaseOffset = pagenum;
				memset(newpb->PhysAddrs, 0, sizeof(newpb->PhysAddrs));
				pb->Next = newpb;
			}
	
			pb = pb->Next;
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

int VFS_MMap_MapPage(tVFS_Node *Node, unsigned int pagenum, tVFS_MMapPageBlock *pb, void *mapping_dest, unsigned int Protection)
{
	if( pb->PhysAddrs[pagenum - pb->BaseOffset] != 0 )
	{
		MM_Map( mapping_dest, pb->PhysAddrs[pagenum - pb->BaseOffset] );
		MM_RefPhys( pb->PhysAddrs[pagenum - pb->BaseOffset] );
		LOG("Cached map %X to %p (%P)", pagenum*PAGE_SIZE, mapping_dest,
			pb->PhysAddrs[pagenum - pb->BaseOffset]);
	}
	else
	{
		tVFS_NodeType	*nt = Node->Type;
		if( !nt ) 
		{
			// TODO: error
		}
		else if( nt->MMap )
			nt->MMap(Node, pagenum*PAGE_SIZE, PAGE_SIZE, mapping_dest);
		else
		{
			 int	read_len;
			// Allocate pages and read data
			if( MM_Allocate(mapping_dest) == 0 ) {
				// TODO: Unwrap
				return 1;
			}
			// TODO: Clip read length
			read_len = nt->Read(Node, pagenum*PAGE_SIZE, PAGE_SIZE, mapping_dest, 0);
			// TODO: This was commented out, why?
			if( read_len != PAGE_SIZE ) {
				memset( (char*)mapping_dest + read_len, 0, PAGE_SIZE-read_len );
			}
		}
		pb->PhysAddrs[pagenum - pb->BaseOffset] = MM_GetPhysAddr( mapping_dest );
		MM_SetPageNode( pb->PhysAddrs[pagenum - pb->BaseOffset], Node );
		MM_RefPhys( pb->PhysAddrs[pagenum - pb->BaseOffset] );
		LOG("Read and map %X to %p (%P)", pagenum*PAGE_SIZE, mapping_dest,
			pb->PhysAddrs[pagenum - pb->BaseOffset]);
	}
	// TODO: Huh?
	Node->ReferenceCount ++;

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
	
	return 0;
}

int VFS_MUnmap(void *Addr, size_t Length)
{
	return 0;
}
