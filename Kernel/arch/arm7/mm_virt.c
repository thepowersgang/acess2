/*
 * Acess2
 * 
 * ARM7 Virtual Memory Manager
 * - arch/arm7/mm_virt.c
 */
#include <mm_virt.h>

// === TYPES ===
typedef struct
{
	tPAddr	PhysAddr;
	Uint8	Size;
	Uint8	Domain;
} tMM_PageInfo;

// === GLOBALS ===
Uint32	*gMM_KernelTable0 = (void*)MM_TABLE0KERN;
Uint32	*gMM_KernelTable1 = (void*)MM_TABLE1KERN;

// === CODE ===
int MM_InitialiseVirtual(void)
{
}

int MM_int_GetPageInfo(tVAddr VAddr, tMM_PageInfo *pi)
{
	Uint32	*table0, table1;
	Uint32	desc;

	if(VAddr & 0x80000000 ) {
		table0 = MM_TABLE0KERN;
		table1 = MM_TABLE1KERN;
	}
	else {
		table0 = MM_TABLE0USER;
		table1 = MM_TABLE1USER;
	}
	VAddr & 0x7FFFFFFF;

	desc = table0[ VAddr >> 20 ];

	switch( (desc & 3) )
	{
	// 0: Unmapped
	case 0:
		pi->PhysAddr = 0;
		pi->Size = 20;
		pi->Domain = 0;
		return 1;

	// 1: Coarse page table
	case 1:
		// Domain from top level table
		pi->Domain = (desc >> 5) & 7;
		// Get next level
		desc = table1[ VAddr >> 12 ];
		switch( desc & 3 )
		{
		// 0: Unmapped
		case 0:	
			pi->Size = 12;
			return 1;
		// 1: Large Page (64KiB)
		case 1:
			pi->Size = 16;
			pi->PhysAddr = desc & 0xFFFF0000;
			return 0;
		// 2/3: Small page
		case 2:
		case 3:
			pi->Size = 12;
			pi->PhysAddr = desc & 0xFFFFF000;
			pi->bExecutable = desc & 1;
			pi->bGlobal = !(desc >> 11);
			pi->bSharec = (desc >> 10) & 1;
			return 1;
		}
		return 1;
	
	// 2: Section (or Supersection)
	case 2:
		if( desc & (1 << 18) ) {
			// Supersection
			pi->PhysAddr = desc & 0xFF000000;
			pi->PhysAddr |= ((desc >> 20) & 0xF) << 32;
			pi->PhysAddr |= ((desc >> 5) & 0x7) << 36;
			pi->Size = 24;
			pi->Domain = 0;	// Superpages default to zero
			return 0;
		}
		
		// Section
		pi->PhysAddr = desc & 0xFFF80000;
		pi->Size = 20;
		pi->Domain = (desc >> 5) & 7;
		return 0;

	// 3: Reserved (invalid)
	case 3:
		pi->PhysAddr = 0;
		pi->Size = 20;
		pi->Domain = 0;
		return 2;
	}
}

// --- Exports ---
tPAddr MM_GetPhysAddr(tVAddr VAddr)
{
	tMM_PageInfo	pi;
	if( MM_int_GetPageInfo(VAddr, &pi) )
		return 0;
	return pi.PhysAddr;
}
