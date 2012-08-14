/*
 * Acess2 Kernel x86 Port
 * - By John Hodge (thePowersGang)
 *
 * mboot.c
 * - Multiboot Support
 */
#define DEBUG	0
#include <acess.h>
#include <mboot.h>

// === CODE ===
int Multiboot_LoadMemoryMap(tMBoot_Info *MBInfo, tVAddr MapOffset, tPMemMapEnt *Map, const int MapSize, tPAddr KStart, tPAddr KEnd)
{
	 int	nPMemMapEnts = 0;
	tMBoot_MMapEnt	*ent = (void*)((tVAddr)MBInfo->MMapAddr + MapOffset);
	tMBoot_MMapEnt	*last = (void*)((tVAddr)ent + MBInfo->MMapLength);
	
	ENTER("pMBInfo pMapOffset pMap iMapSize PKStart PKEnd",
		MBInfo, MapOffset, Map, MapSize, KStart, KEnd);

	// Build up memory map
	nPMemMapEnts = 0;
	while( ent < last && nPMemMapEnts < MapSize )
	{
		tPMemMapEnt	*nent = &Map[nPMemMapEnts];
		nent->Start = ent->Base;
		nent->Length = ent->Length;
		switch(ent->Type)
		{
		case 1:
			nent->Type = PMEMTYPE_FREE;
			break;
		default:
			nent->Type = PMEMTYPE_RESERVED;
			break;
		}
		nent->NUMADomain = 0;
		
		nPMemMapEnts ++;
		ent = (void*)( (tVAddr)ent + ent->Size + 4 );
	}

	// Ensure it's valid
	nPMemMapEnts = PMemMap_ValidateMap(Map, nPMemMapEnts, MapSize);
	// TODO: Error handling

	// Replace kernel with PMEMTYPE_USED
	nPMemMapEnts = PMemMap_MarkRangeUsed(
		Map, nPMemMapEnts, MapSize,
		KStart, KEnd - KStart
		);

	// Replace modules with PMEMTYPE_USED
	nPMemMapEnts = PMemMap_MarkRangeUsed(Map, nPMemMapEnts, MapSize,
		MBInfo->Modules, MBInfo->ModuleCount*sizeof(tMBoot_Module)
		);
	tMBoot_Module *mods = (void*)( (tVAddr)MBInfo->Modules + MapOffset);
	for( int i = 0; i < MBInfo->ModuleCount; i ++ )
	{
		nPMemMapEnts = PMemMap_MarkRangeUsed(
			Map, nPMemMapEnts, MapSize,
			mods->Start, mods->End - mods->Start
			);
	}
	
	// Debug - Output map
	PMemMap_DumpBlocks(Map, nPMemMapEnts);

	LEAVE('i', nPMemMapEnts);
	return nPMemMapEnts;
}

tBootModule *Multiboot_LoadModules(tMBoot_Info *MBInfo, tVAddr MapOffset, int *ModuleCount)
{
	tMBoot_Module	*mods = (void*)( MBInfo->Modules + MapOffset );
	*ModuleCount = MBInfo->ModuleCount;
	tBootModule *ret = malloc( MBInfo->ModuleCount * sizeof(*ret) );
	for( int i = 0; i < MBInfo->ModuleCount; i ++ )
	{
		 int	ofs;
	
		Log_Log("Arch", "Multiboot Module at 0x%08x, 0x%08x bytes (String at 0x%08x)",
			mods[i].Start, mods[i].End-mods[i].Start, mods[i].String);
	
		ret[i].PBase = mods[i].Start;
		ret[i].Size = mods[i].End - mods[i].Start;
	
		// Always HW map the module data	
		ofs = mods[i].Start&0xFFF;
		ret[i].Base = (void*)( MM_MapHWPages(mods[i].Start,
			(ret[i].Size+ofs+0xFFF) / 0x1000
			) + ofs );
		
		// Only map the string if needed
		if( !MM_GetPhysAddr( (void*)(mods[i].String + MapOffset) ) )
		{
			// Assumes the string is < 4096 bytes long)
			ret[i].ArgString = (void*)(
				MM_MapHWPages(mods[i].String, 2) + (mods[i].String&0xFFF)
				);
		}
		else
			ret[i].ArgString = (char*)(tVAddr)mods[i].String + MapOffset;
	}

	return ret;
}

