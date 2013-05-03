/*
 * Acess2 Kernel x86 Port
 * - By John Hodge (thePowersGang)
 *
 * mboot.c
 * - Multiboot Support
 */
#define DEBUG	1
#include <acess.h>
#include <mboot.h>

// === CODE ===
int Multiboot_LoadMemoryMap(tMBoot_Info *MBInfo, tVAddr MapOffset, tPMemMapEnt *Map, const int MapSize, tPAddr KStart, tPAddr KEnd)
{
	 int	nPMemMapEnts = 0;
	
	ENTER("pMBInfo pMapOffset pMap iMapSize PKStart PKEnd",
		MBInfo, MapOffset, Map, MapSize, KStart, KEnd);

	// Check that the memory map is present
	if( MBInfo->Flags & (1 << 6) )
	{
		tMBoot_MMapEnt	*ent = (void*)((tVAddr)MBInfo->MMapAddr + MapOffset);
		tMBoot_MMapEnt	*last = (void*)((tVAddr)ent + MBInfo->MMapLength);
		// Build up memory map
		nPMemMapEnts = 0;
		while( ent < last && nPMemMapEnts < MapSize )
		{
			tPMemMapEnt	*nent = &Map[nPMemMapEnts];
			if( !MM_GetPhysAddr(ent) )
				Log_KernelPanic("MBoot", "MBoot Map entry %i addres bad (%p)",
					nPMemMapEnts, ent);
			LOG("%llx+%llx", ent->Base, ent->Length);
	
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
		if( ent < last )
		{
			Log_Warning("MBoot", "Memory map has >%i entries, internal version is truncated",
				MapSize);
		}
	}
	else if( MBInfo->Flags & (1 << 0) )
	{
		Log_Warning("MBoot", "No memory map passed, using mem_lower and mem_upper");
		ASSERT(MapSize >= 2);
		nPMemMapEnts = 2;
		Map[0].Start = 0;
		Map[0].Length = MBInfo->LowMem * 1024;
		Map[0].Type = PMEMTYPE_FREE;
		Map[0].NUMADomain = 0;

		Map[1].Start = 0x100000;
		Map[1].Length = MBInfo->HighMem * 1024;
		Map[1].Type = PMEMTYPE_FREE;
		Map[1].NUMADomain = 0;
	}
	else
	{
		Log_KernelPanic("MBoot", "Multiboot didn't pass memory information");
	}

	// Ensure it's valid
	LOG("Validating");
	nPMemMapEnts = PMemMap_ValidateMap(Map, nPMemMapEnts, MapSize);
	// TODO: Error handling

	// Replace kernel with PMEMTYPE_USED
	LOG("Marking kernel");
	nPMemMapEnts = PMemMap_MarkRangeUsed(
		Map, nPMemMapEnts, MapSize,
		KStart, KEnd - KStart
		);

	LOG("Dumping");
	PMemMap_DumpBlocks(Map, nPMemMapEnts);

	// Check if boot modules were passed
	if( MBInfo->Flags & (1 << 3) )
	{
		// Replace modules with PMEMTYPE_USED
		nPMemMapEnts = PMemMap_MarkRangeUsed(Map, nPMemMapEnts, MapSize,
			MBInfo->Modules, MBInfo->ModuleCount*sizeof(tMBoot_Module)
			);
		LOG("MBInfo->Modules = %x", MBInfo->Modules);
		tMBoot_Module *mods = (void*)( (tVAddr)MBInfo->Modules + MapOffset);
		for( int i = 0; i < MBInfo->ModuleCount; i ++ )
		{
			LOG("&mods[%i] = %p", i, &mods[i]);
			LOG("mods[i] = {0x%x -> 0x%x}", mods[i].Start, mods[i].End);
			nPMemMapEnts = PMemMap_MarkRangeUsed(
				Map, nPMemMapEnts, MapSize,
				mods[i].Start, mods[i].End - mods[i].Start
				);
		}
	}
		
	// Debug - Output map
	PMemMap_DumpBlocks(Map, nPMemMapEnts);

	LEAVE('i', nPMemMapEnts);
	return nPMemMapEnts;
}

tBootModule *Multiboot_LoadModules(tMBoot_Info *MBInfo, tVAddr MapOffset, int *ModuleCount)
{
	if( !(MBInfo->Flags & (1 << 3)) ) {
		*ModuleCount = 0;
		Log_Log("Arch", "No multiboot module information passed");
		return NULL;
	}

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

