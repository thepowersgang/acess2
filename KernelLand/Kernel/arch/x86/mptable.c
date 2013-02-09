/*
 * Acess2 Kernel (x86 Port)
 * - By John Hodge (thePowersGang)
 *
 * mptable.c
 * - MP Table parsing
 */
#include <proc_int.h>
#include <mptable.h>

// === PROTOTYPES ===
const void	*MPTable_int_LocateFloatPtr(const void *Base, size_t Size);

// === CODE ===
const void *MPTable_int_LocateFloatPtr(const void *Base, size_t Size)
{
	const Uint32	*ptr = Base;

	ASSERT( !(Size & (16-1)) );

	for( ptr = Base; Size -= 16; ptr += 16/4 )
	{
		if( *ptr != MPPTR_IDENT )
			continue ;
		Log("Possible table at %p", ptr);
		if( ByteSum(ptr, sizeof(tMPInfo)) != 0 )
			continue ;
	
		return ptr;
	}
	return NULL;
}

const void *MPTable_LocateFloatPtr(void)
{
	const void *ret;
	// Find MP Floating Table
	
	// - EBDA/Last 1Kib (640KiB)
	ret = MPTable_int_LocateFloatPtr( (void*)(KERNEL_BASE|0x9F000), 0x1000 );
	if(ret)	return ret;
	
	// - Last KiB (512KiB base mem)
	ret = MPTable_int_LocateFloatPtr( (void*)(KERNEL_BASE|0x7F000), 0x1000 );
	if(ret)	return ret;

	// - BIOS ROM
	ret = MPTable_int_LocateFloatPtr( (void*)(KERNEL_BASE|0xE0000), 0x20000 );
	if(ret)	return ret;
	
	return NULL;
}

int MPTable_FillCPUs(const void *FloatPtr, tCPU *CPUs, int MaxCPUs, int *BSPIndex)
{
	 int	cpu_count = 0;
	
	const tMPInfo	*mpfloatptr = FloatPtr;
	#if DUMP_MP_TABLE
	Log("*mpfloatptr = %p {", mpfloatptr);
	Log("\t.Sig = 0x%08x", mpfloatptr->Sig);
	Log("\t.MPConfig = 0x%08x", mpfloatptr->MPConfig);
	Log("\t.Length = 0x%02x", mpfloatptr->Length);
	Log("\t.Version = 0x%02x", mpfloatptr->Version);
	Log("\t.Checksum = 0x%02x", mpfloatptr->Checksum);
	Log("\t.Features = [0x%02x,0x%02x,0x%02x,0x%02x,0x%02x]",
		mpfloatptr->Features[0],	mpfloatptr->Features[1],
		mpfloatptr->Features[2],	mpfloatptr->Features[3],
		mpfloatptr->Features[4]
		);
	Log("}");
	#endif		

	const tMPTable *mptable = (void*)( KERNEL_BASE|mpfloatptr->MPConfig );
	#if DUMP_MP_TABLE
	Log("*mptable = %p {", mptable);
	Log("\t.Sig = 0x%08x", mptable->Sig);
	Log("\t.BaseTableLength = 0x%04x", mptable->BaseTableLength);
	Log("\t.SpecRev = 0x%02x", mptable->SpecRev);
	Log("\t.Checksum = 0x%02x", mptable->Checksum);
	Log("\t.OEMID = '%8c'", mptable->OemID);
	Log("\t.ProductID = '%8c'", mptable->ProductID);
	Log("\t.OEMTablePtr = %p'", mptable->OEMTablePtr);
	Log("\t.OEMTableSize = 0x%04x", mptable->OEMTableSize);
	Log("\t.EntryCount = 0x%04x", mptable->EntryCount);
	Log("\t.LocalAPICMemMap = 0x%08x", mptable->LocalAPICMemMap);
	Log("\t.ExtendedTableLen = 0x%04x", mptable->ExtendedTableLen);
	Log("\t.ExtendedTableChecksum = 0x%02x", mptable->ExtendedTableChecksum);
	Log("}");
	#endif
	
	gpMP_LocalAPIC = (void*)MM_MapHWPages(mptable->LocalAPICMemMap, 1);
	
	const tMPTable_Ent	*ents = mptable->Entries;
	
	cpu_count = 0;
	for( int i = 0; i < mptable->EntryCount; i ++ )
	{
		size_t	entSize = 4;
		switch( ents->Type )
		{
		case 0:	// Processor
			entSize = 20;
			#if DUMP_MP_TABLE
			Log("%i: Processor", i);
			Log("\t.APICID = %i", ents->Proc.APICID);
			Log("\t.APICVer = 0x%02x", ents->Proc.APICVer);
			Log("\t.CPUFlags = 0x%02x", ents->Proc.CPUFlags);
			Log("\t.CPUSignature = 0x%08x", ents->Proc.CPUSignature);
			Log("\t.FeatureFlags = 0x%08x", ents->Proc.FeatureFlags);
			#endif
			
			if( !(ents->Proc.CPUFlags & 1) ) {
				Log("DISABLED");
				break;
			}
			
			// Check if there is too many processors
			if((cpu_count) >= MaxCPUs) {
				(cpu_count) ++;	// If `giNumCPUs` > MaxCPUs later, it will be clipped
				break;
			}
			
			// Initialise CPU Info
			CPUs[cpu_count].APICID = ents->Proc.APICID;
			CPUs[cpu_count].State = 0;
			
			// Set BSP Variable
			if( ents->Proc.CPUFlags & 2 )
			{
				if( *BSPIndex >= 0 )
					Warning("Multiple CPUs with the BSP flag set, %i+%i", *BSPIndex, cpu_count);
				else
					*BSPIndex = cpu_count;
			}
			
			(cpu_count) ++;
			break;
		
		#if DUMP_MP_TABLE >= 2
		case 1:	// Bus
			entSize = 8;
			Log("%i: Bus", i);
			Log("\t.ID = %i", ents->Bus.ID);
			Log("\t.TypeString = '%6C'", ents->Bus.TypeString);
			break;
		case 2:	// I/O APIC
			entSize = 8;
			Log("%i: I/O APIC", i);
			Log("\t.ID = %i", ents->IOAPIC.ID);
			Log("\t.Version = 0x%02x", ents->IOAPIC.Version);
			Log("\t.Flags = 0x%02x", ents->IOAPIC.Flags);
			Log("\t.Addr = 0x%08x", ents->IOAPIC.Addr);
			break;
		case 3:	// I/O Interrupt Assignment
			entSize = 8;
			Log("%i: I/O Interrupt Assignment", i);
			Log("\t.IntType = %i", ents->IOInt.IntType);
			Log("\t.Flags = 0x%04x", ents->IOInt.Flags);
			Log("\t.SourceBusID = 0x%02x", ents->IOInt.SourceBusID);
			Log("\t.SourceBusIRQ = 0x%02x", ents->IOInt.SourceBusIRQ);
			Log("\t.DestAPICID = 0x%02x", ents->IOInt.DestAPICID);
			Log("\t.DestAPICIRQ = 0x%02x", ents->IOInt.DestAPICIRQ);
			break;
		case 4:	// Local Interrupt Assignment
			entSize = 8;
			Log("%i: Local Interrupt Assignment", i);
			Log("\t.IntType = %i", ents->LocalInt.IntType);
			Log("\t.Flags = 0x%04x", ents->LocalInt.Flags);
			Log("\t.SourceBusID = 0x%02x", ents->LocalInt.SourceBusID);
			Log("\t.SourceBusIRQ = 0x%02x", ents->LocalInt.SourceBusIRQ);
			Log("\t.DestLocalAPICID = 0x%02x", ents->LocalInt.DestLocalAPICID);
			Log("\t.DestLocalAPICIRQ = 0x%02x", ents->LocalInt.DestLocalAPICIRQ);
			break;
		default:
			Log("%i: Unknown (%i)", i, ents->Type);
			break;
		#endif
		}
		ents = (void*)( (Uint)ents + entSize );
	}
	
	if( cpu_count > MaxCPUs )
	{
		Warning("Too many CPUs detected (%i), only using %i of them", cpu_count, MaxCPUs);
		cpu_count = MaxCPUs;
	}
	
	return cpu_count;
}

