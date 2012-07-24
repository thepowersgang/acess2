/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * pmemmap.h
 * - Physical Memory Map definitions
 */
#ifndef _PMEMMAP_H_
#define _PMEMMAP_H_

typedef struct sPMemMapEnt	tPMemMapEnt;

enum ePMemMapEntType
{
	PMEMTYPE_FREE,	// Free RAM
	PMEMTYPE_USED,	// Used by Kernel / Modules
	PMEMTYPE_RESERVED,	// Unavaliable
	PMEMTYPE_NVRAM,	// Non-volatile
	PMEMTYPE_UNMAPPED	// Nothing on these lines
};

struct sPMemMapEnt
{
	Uint64	Start;
	Uint64	Length;
	enum ePMemMapEntType	Type;
	Uint16	NUMADomain;
};

extern void	PMemMap_DumpBlocks(tPMemMapEnt *map, int NEnts);
extern int	PMemMap_SplitBlock(tPMemMapEnt *map, int NEnts, int MaxEnts, int Block, Uint64 Offset);
extern int	PMemMap_CompactMap(tPMemMapEnt *map, int NEnts, int MaxEnts);
extern int	PMemMap_ValidateMap(tPMemMapEnt *map, int NEnts, int MaxEnts);
extern int	PMemMap_MarkRangeUsed(tPMemMapEnt *map, int NEnts, int MaxEnts, Uint64 Base, Uint64 Size);

#endif

