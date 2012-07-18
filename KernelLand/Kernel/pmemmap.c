/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * pmemmap.c
 * - Physical memory map manipulation
 */
#include <acess.h>
#include <pmemmap.h>

// === CODE ===
void PMemMap_DumpBlocks(tPMemMapEnt *map, int NEnts)
{
	for( int i = 0; i < NEnts; i ++ )
	{
		Log_Debug("Arch", "%i: %i 0x%02x %08llx+%llx",
			i, map[i].Type, map[i].NUMADomain,
			map[i].Start, map[i].Length
			);
	}
}

int PMemMap_SplitBlock(tPMemMapEnt *map, int NEnts, int MaxEnts, int Block, Uint64 Offset)
{
	LOG("Splitting %i (%llx+%llx) at %llx", Block, map[Block].Start, map[Block].Length, Offset);
	
	Uint64 _len = map[Block].Length;
	map[Block].Length = Offset;
	if( NEnts == MaxEnts ) {
		// out of space
		return NEnts;
	}
	Block ++;
	if( Block < NEnts ) {
		// Can't be anything after
		memmove(&map[Block+1], &map[Block], (NEnts - Block)*sizeof(map[0]));
	}
	NEnts ++;
	
	// New (free) block
	map[Block].Start  = map[Block-1].Start + Offset;
	map[Block].Length = _len - Offset;
	map[Block].Type = map[Block-1].Type;
	map[Block].NUMADomain = map[Block-1].NUMADomain;
	LOG("- New %i %02x %llx+%llx", map[Block].Type, map[Block].NUMADomain, map[Block].Start, map[Block].Length);

	return NEnts;
}

int PMemMap_CompactMap(tPMemMapEnt *map, int NEnts, int MaxEnts)
{
	for( int i = 1; i < NEnts; i ++ )
	{
		// Check if the ranges are contiguous
		if( map[i-1].Start + map[i-1].Length < map[i].Start )
			continue ;
		// Check if the type is the same
		if( map[i-1].Type != map[i].Type )
			continue ;
		// Check if the NUMA Domains are the same
		if( map[i-1].NUMADomain != map[i].NUMADomain )
			continue ;
		
		// Ok, they should be together
		map[i-1].Length += map[i].Length;
		memmove(&map[i], &map[i+1], (NEnts - (i+1))*sizeof(map[0]));
		
		// Counteract the i++ in the loop iterator
		i --;
		NEnts --;
	}
	return NEnts;
}

int PMemMap_ValidateMap(tPMemMapEnt *map, int NEnts, int MaxEnts)
{
	// Sort the pmem map
	 int	 bNeedsSort = 0;
	for( int i = 1; i < NEnts; i ++ )
	{
		if( map[i-1].Start > map[i].Start ) {
			bNeedsSort = 1;
			break;
		}
	}
	if( bNeedsSort )
	{
		Log_Warning("Arch", "TODO: Impliment memory map sorting");
		// TODO: Sort memory map
	}
	
	// Ensure that the map has no overlaps
	for( int i = 1; i < NEnts; i ++ )
	{
		if( map[i-1].Start + map[i-1].Length <= map[i].Start )
			continue ;
		// Oops, overlap!
		Log_Notice("Arch", "Map ranges %llx+%llx and %llx+%llx overlap",
			map[i-1].Start, map[i-1].Length,
			map[i].Start,   map[i].Length
			);
	}
	
	return PMemMap_CompactMap(map, NEnts, MaxEnts);
}


int PMemMap_MarkRangeUsed(tPMemMapEnt *map, int NEnts, int MaxEnts, Uint64 Base, Uint64 Size)
{
	 int	first;
	
	Size = (Size + 0xFFF) & ~0xFFF;
	Base = Base & ~0xFFF;
	
	first = -1;
	for( int i = 0; i < NEnts; i ++ )
	{
		if( map[i].Start + map[i].Length > Base ) {
			first = i;
			break;
		}
	}
	if( first == -1 ) {
		// Not in map
		LOG("%llx+%llx not in map (past end)", Base, Size);
		return NEnts;
	}
	
	if( map[first].Start > Base ) {
		// Not in map
		LOG("%llx+%llx not in map (in hole)", Base, Size);
		return NEnts;
	}
	
	// Detect single
	if( map[first].Start <= Base && Base + Size <= map[first].Start + map[first].Length )
	{
		// Split before
		if( map[first].Start < Base )
		{
			if( NEnts == MaxEnts ) {
				// out of space... oops
				return NEnts;
			}
			NEnts = PMemMap_SplitBlock(map, NEnts, MaxEnts, first, Base - map[first].Start);
			first ++;
		}
		
		// map[first].Start == Base
		// Split after
		if( map[first].Length > Size )
		{
			if( NEnts == MaxEnts ) {
				// out of space
				return NEnts;
			}
			NEnts = PMemMap_SplitBlock(map, NEnts, MaxEnts, first, Size);
		}
		
		// map[first] is now exactly the block
		map[first].Type = PMEMTYPE_USED;
	
		return PMemMap_CompactMap(map, NEnts, MaxEnts);
	}
	else
	{
		// Wait... this should never happen, right?
		Log_Notice("Arch", "Module %llx+%llx overlaps two or more ranges",
			Base, Size);
		PMemMap_DumpBlocks(map, NEnts);
		// TODO: Error?
		return NEnts;
	}
}



