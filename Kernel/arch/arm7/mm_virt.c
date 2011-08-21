/*
 * Acess2
 * 
 * ARM7 Virtual Memory Manager
 * - arch/arm7/mm_virt.c
 */
#include <acess.h>
#include <mm_virt.h>

#define AP_KRW_ONLY	0x1
#define AP_KRO_ONLY	0x5
#define AP_RW_BOTH	0x3
#define AP_RO_BOTH	0x6

// === TYPES ===
typedef struct
{
	tPAddr	PhysAddr;
	Uint8	Size;
	Uint8	Domain;
	BOOL	bExecutable;
	BOOL	bGlobal;
	BOOL	bShared;
	 int	AP;
} tMM_PageInfo;

// === PROTOTYPES ===
 int	MM_int_SetPageInfo(tVAddr VAddr, tMM_PageInfo *pi);
 int	MM_int_GetPageInfo(tVAddr VAddr, tMM_PageInfo *pi);

// === GLOBALS ===

// === CODE ===
int MM_InitialiseVirtual(void)
{
	return 0;
}

int MM_int_SetPageInfo(tVAddr VAddr, tMM_PageInfo *pi)
{
	Uint32	*table0, *table1;
	Uint32	*desc;

	if(VAddr & 0x80000000 ) {
		table0 = (void*)MM_TABLE0KERN;	// Level 0
		table1 = (void*)MM_TABLE1KERN;	// Level 1
	}
	else {
		table0 = (void*)MM_TABLE0USER;
		table1 = (void*)MM_TABLE1USER;
	}
	VAddr &= 0x7FFFFFFF;

	desc = &table0[ VAddr >> 20 ];

	switch(pi->Size)
	{
	case 12:	// Small Page
	case 16:	// Large Page
		if( (*desc & 3) == 0 ) {
			if( pi->PhysAddr == 0 )	return 0;
			// Allocate
			*desc = MM_AllocPhys();
			*desc |= pi->Domain << 5;
			*desc |= 1;
		}
		desc = &table1[ VAddr >> 12 ];
		if( pi->Size == 12 )
		{
			// Small page
			// - Error if overwriting a large page
			if( (*desc & 3) == 1 )	return 1;
			if( pi->PhysAddr == 0 ) {
				*desc = 0;
				return 0;
			}

			*desc = (pi->PhysAddr & 0xFFFFF000) | 2;
			if(!pi->bExecutable)	*desc |= 1;	// XN
			if(!pi->bGlobal)	*desc |= 1 << 11;	// NG
			if( pi->bShared)	*desc |= 1 << 10;	// S
			*desc |= (pi->AP & 3) << 4;	// AP
			*desc |= ((pi->AP >> 2) & 1) << 9;	// APX
		}
		else
		{
			// Large page
			// TODO: 
		}
		break;
	case 20:	// Section or unmapped
		Log_Warning("MM", "TODO: Implement sections");
		break;
	case 24:	// Supersection
		// Error if not aligned
		if( VAddr & 0xFFFFFF ) {
			return 1;
		}
		if( (*desc & 3) == 0 || ((*desc & 3) == 2 && (*desc & (1 << 18)))  )
		{
			if( pi->PhysAddr == 0 ) {
				*desc = 0;
				// TODO: Apply to all entries
				return 0;
			}
			// Apply
			*desc = pi->PhysAddr & 0xFF000000;
//			*desc |= ((pi->PhysAddr >> 32) & 0xF) << 20;
//			*desc |= ((pi->PhysAddr >> 36) & 0x7) << 5;
			*desc |= 2 | (1 << 18);
			// TODO: Apply to all entries
			return 0;
		}
		return 1;
	}

	return 1;
}

int MM_int_GetPageInfo(tVAddr VAddr, tMM_PageInfo *pi)
{
	Uint32	*table0, *table1;
	Uint32	desc;

	if(VAddr & 0x80000000 ) {
		table0 = (void*)MM_TABLE0KERN;	// Level 0
		table1 = (void*)MM_TABLE1KERN;	// Level 1
	}
	else {
		table0 = (void*)MM_TABLE0USER;
		table1 = (void*)MM_TABLE1USER;
	}
	VAddr &= 0x7FFFFFFF;

	desc = table0[ VAddr >> 20 ];
	
	pi->bExecutable = 1;
	pi->bGlobal = 0;
	pi->bShared = 0;

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
			pi->bShared = (desc >> 10) & 1;
			return 1;
		}
		return 1;
	
	// 2: Section (or Supersection)
	case 2:
		if( desc & (1 << 18) ) {
			// Supersection
			pi->PhysAddr = desc & 0xFF000000;
			pi->PhysAddr |= (Uint64)((desc >> 20) & 0xF) << 32;
			pi->PhysAddr |= (Uint64)((desc >> 5) & 0x7) << 36;
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
	return 2;
}

// --- Exports ---
tPAddr MM_GetPhysAddr(tVAddr VAddr)
{
	tMM_PageInfo	pi;
	if( MM_int_GetPageInfo(VAddr, &pi) )
		return 0;
	return pi.PhysAddr;
}

Uint MM_GetFlags(tVAddr VAddr)
{
	tMM_PageInfo	pi;
	 int	ret;

	if( MM_int_GetPageInfo(VAddr, &pi) )
		return 0;

	ret = 0;
	
	switch(pi.AP)
	{
	case AP_KRW_ONLY:
		ret |= MM_PFLAG_KERNEL;
		break;
	case AP_KRO_ONLY:
		ret |= MM_PFLAG_KERNEL|MM_PFLAG_RO;
		break;
	case AP_RW_BOTH:
		break;
	case AP_RO_BOTH:
		ret |= MM_PFLAG_RO;
		break;
	}

	if( pi.bExecutable )	ret |= MM_PFLAG_EXEC;
	return ret;
}

void MM_SetFlags(tVAddr VAddr, Uint Flags, Uint Mask)
{
	tMM_PageInfo	pi;
	if( MM_int_GetPageInfo(VAddr, &pi) )
		return;
	
		

}

int MM_Map(tVAddr VAddr, tPAddr PAddr)
{
	tMM_PageInfo	pi = {0};
	pi.PhysAddr = PAddr;
	pi.Size = 12;
	pi.AP = AP_KRW_ONLY;	// Kernel Read/Write
	pi.bExecutable = 1;
	if( MM_int_SetPageInfo(VAddr, &pi) ) {
		MM_DerefPhys(pi.PhysAddr);
		return 0;
	}
	return pi.PhysAddr;
}

tPAddr MM_Allocate(tVAddr VAddr)
{
	tMM_PageInfo	pi = {0};

	pi.PhysAddr = MM_AllocPhys();
	if( pi.PhysAddr == 0 )	return 0;
	pi.Size = 12;
	pi.AP = AP_KRW_ONLY;	// Kernel Read/Write
	pi.bExecutable = 1;
	if( MM_int_SetPageInfo(VAddr, &pi) ) {
		MM_DerefPhys(pi.PhysAddr);
		return 0;
	}
	return pi.PhysAddr;
}

void MM_Deallocate(tVAddr VAddr)
{
	tMM_PageInfo	pi;
	
	if( MM_int_GetPageInfo(VAddr, &pi) )	return ;

	if( pi.PhysAddr == 0 )	return;
	MM_DerefPhys(pi.PhysAddr);
	
	pi.PhysAddr = 0;
	pi.AP = 0;
	pi.bExecutable = 0;
	MM_int_SetPageInfo(VAddr, &pi);
}
