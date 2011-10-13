/*
 * Acess2
 * 
 * ARM7 Virtual Memory Manager
 * - arch/arm7/mm_virt.c
 */
#define DEBUG	0
#include <acess.h>
#include <mm_virt.h>
#include <hal_proc.h>

#define AP_KRW_ONLY	0x1
#define AP_KRO_ONLY	0x5
#define AP_RW_BOTH	0x3
#define AP_RO_BOTH	0x6
#define PADDR_MASK_LVL1	0xFFFFFC00

// === IMPORTS ===
extern Uint32	kernel_table0[];

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

//#define FRACTAL(table1, addr)	((table1)[ (0xFF8/4*1024) + ((addr)>>20)])
#define FRACTAL(table1, addr)	((table1)[ (0xFF8/4*1024) + ((addr)>>22)])
#define USRFRACTAL(table1, addr)	((table1)[ (0x7F8/4*1024) + ((addr)>>22)])
#define TLBIALL()	__asm__ __volatile__ ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0))

// === PROTOTYPES ===
void	MM_int_GetTables(tVAddr VAddr, Uint32 **Table0, Uint32 **Table1);
 int	MM_int_AllocateCoarse(tVAddr VAddr, int Domain);
 int	MM_int_SetPageInfo(tVAddr VAddr, tMM_PageInfo *pi);
 int	MM_int_GetPageInfo(tVAddr VAddr, tMM_PageInfo *pi);
tPAddr	MM_AllocateRootTable(void);
void	MM_int_CloneTable(Uint32 *DestEnt, int Table);
tPAddr	MM_Clone(void);
tVAddr	MM_NewKStack(int bGlobal);
void	MM_int_DumpTableEnt(tVAddr Start, size_t Len, tMM_PageInfo *Info);
//void	MM_DumpTables(tVAddr Start, tVAddr End);

// === GLOBALS ===

// === CODE ===
int MM_InitialiseVirtual(void)
{
	return 0;
}

void MM_int_GetTables(tVAddr VAddr, Uint32 **Table0, Uint32 **Table1)
{
	if(VAddr & 0x80000000) {
		*Table0 = (void*)&kernel_table0;	// Level 0
		*Table1 = (void*)MM_TABLE1KERN;	// Level 1
	}
	else {
		*Table0 = (void*)MM_TABLE0USER;
		*Table1 = (void*)MM_TABLE1USER;
	}
}

int MM_int_AllocateCoarse(tVAddr VAddr, int Domain)
{
	Uint32	*table0, *table1;
	Uint32	*desc;
	tPAddr	paddr;
	
	ENTER("xVAddr iDomain", VAddr, Domain);

	MM_int_GetTables(VAddr, &table0, &table1);

	VAddr &= ~(0x400000-1);	// 4MiB per "block", 1 Page

	desc = &table0[ VAddr>>20];
	LOG("desc = %p", desc);
	
	// table0: 4 bytes = 1 MiB

	LOG("desc[0] = %x", desc[0]);
	LOG("desc[1] = %x", desc[1]);
	LOG("desc[2] = %x", desc[2]);
	LOG("desc[3] = %x", desc[3]);

	if( (desc[0] & 3) != 0 || (desc[1] & 3) != 0
	 || (desc[2] & 3) != 0 || (desc[3] & 3) != 0 )
	{
		// Error?
		LEAVE('i', 1);
		return 1;
	}

	paddr = MM_AllocPhys();
	if( !paddr )
	{
		// Error
		LEAVE('i', 2);
		return 2;
	}
	
	*desc = paddr | (Domain << 5) | 1;
	desc[1] = desc[0] + 0x400;
	desc[2] = desc[0] + 0x800;
	desc[3] = desc[0] + 0xC00;

	FRACTAL(table1, VAddr) = paddr | 3;

	// TLBIALL 
	TLBIALL();	

	LEAVE('i', 0);
	return 0;
}	

int MM_int_SetPageInfo(tVAddr VAddr, tMM_PageInfo *pi)
{
	Uint32	*table0, *table1;
	Uint32	*desc;

	ENTER("pVADdr ppi", VAddr, pi);

	MM_int_GetTables(VAddr, &table0, &table1);

	desc = &table0[ VAddr >> 20 ];
	LOG("desc = %p", desc);

	switch(pi->Size)
	{
	case 12:	// Small Page
	case 16:	// Large Page
		LOG("Page");
		if( (*desc & 3) == 0 ) {
			MM_int_AllocateCoarse( VAddr, pi->Domain );
		}
		desc = &table1[ VAddr >> 12 ];
		LOG("desc (2) = %p", desc);
		if( pi->Size == 12 )
		{
			// Small page
			// - Error if overwriting a large page
			if( (*desc & 3) == 1 )	LEAVE_RET('i', 1);
			if( pi->PhysAddr == 0 ) {
				*desc = 0;
				LEAVE('i', 0);
				return 0;
			}

			*desc = (pi->PhysAddr & 0xFFFFF000) | 2;
			if(!pi->bExecutable)	*desc |= 1;	// XN
			if(!pi->bGlobal)	*desc |= 1 << 11;	// NG
			if( pi->bShared)	*desc |= 1 << 10;	// S
			*desc |= (pi->AP & 3) << 4;	// AP
			*desc |= ((pi->AP >> 2) & 1) << 9;	// APX
			LEAVE('i', 0);
			return 0;
		}
		else
		{
			// Large page
			// TODO: 
		}
		break;
	case 20:	// Section or unmapped
		Warning("TODO: Implement sections");
		break;
	case 24:	// Supersection
		// Error if not aligned
		if( VAddr & 0xFFFFFF ) {
			LEAVE('i', 1);
			return 1;
		}
		if( (*desc & 3) == 0 || ((*desc & 3) == 2 && (*desc & (1 << 18)))  )
		{
			if( pi->PhysAddr == 0 ) {
				*desc = 0;
				// TODO: Apply to all entries
				LEAVE('i', 0);
				return 0;
			}
			// Apply
			*desc = pi->PhysAddr & 0xFF000000;
//			*desc |= ((pi->PhysAddr >> 32) & 0xF) << 20;
//			*desc |= ((pi->PhysAddr >> 36) & 0x7) << 5;
			*desc |= 2 | (1 << 18);
			// TODO: Apply to all entries
			LEAVE('i', 0);
			return 0;
		}
		// TODO: What here?
		LEAVE('i', 1);
		return 1;
	}

	LEAVE('i', 1);
	return 1;
}

int MM_int_GetPageInfo(tVAddr VAddr, tMM_PageInfo *pi)
{
	Uint32	*table0, *table1;
	Uint32	desc;

//	LogF("MM_int_GetPageInfo: VAddr=%p, pi=%p\n", VAddr, pi);
	
	MM_int_GetTables(VAddr, &table0, &table1);

	desc = table0[ VAddr >> 20 ];

//	if( VAddr > 0x90000000)
//		LOG("table0 desc(%p) = %x", &table0[ VAddr >> 20 ], desc);
	
	pi->bExecutable = 1;
	pi->bGlobal = 0;
	pi->bShared = 0;
	pi->AP = 0;

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
//		LOG("table1 desc(%p) = %x", &table1[ VAddr >> 12 ], desc);
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
			pi->AP = ((desc >> 4) & 3) | (((desc >> 9) & 1) << 2);
			pi->bExecutable = !(desc & 0x8000);
			pi->bShared = (desc >> 10) & 1;
//			LogF("Large page, VAddr = %p, table1[VAddr>>12] = %p, desc = %x\n", VAddr, &table1[ VAddr >> 12 ], desc);
//			LogF("Par desc = %p %x\n", &table0[ VAddr >> 20 ], table0[ VAddr >> 20 ]);
			return 0;
		// 2/3: Small page
		case 2:
		case 3:
			pi->Size = 12;
			pi->PhysAddr = desc & 0xFFFFF000;
			pi->bExecutable = !(desc & 1);
			pi->bGlobal = !(desc >> 11);
			pi->bShared = (desc >> 10) & 1;
			pi->AP = ((desc >> 4) & 3) | (((desc >> 9) & 1) << 2);
			return 0;
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
			pi->Domain = 0;	// Supersections default to zero
			pi->AP = ((desc >> 10) & 3) | (((desc >> 15) & 1) << 2);
			return 0;
		}
		
		// Section
		pi->PhysAddr = desc & 0xFFF80000;
		pi->Size = 20;
		pi->Domain = (desc >> 5) & 7;
		pi->AP = ((desc >> 10) & 3) | (((desc >> 15) & 1) << 2);
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
	return pi.PhysAddr | (VAddr & ((1 << pi.Size)-1));
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
	
	ENTER("pVAddr", VAddr);

	pi.PhysAddr = MM_AllocPhys();
	if( pi.PhysAddr == 0 )	LEAVE_RET('i', 0);
	pi.Size = 12;
	pi.AP = AP_KRW_ONLY;	// Kernel Read/Write
	pi.bExecutable = 1;
	if( MM_int_SetPageInfo(VAddr, &pi) ) {
		MM_DerefPhys(pi.PhysAddr);
		LEAVE('i', 0);
		return 0;
	}
	LEAVE('x', pi.PhysAddr);
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

tPAddr MM_AllocateRootTable(void)
{
	tPAddr	ret;
	
	ret = MM_AllocPhysRange(2, -1);
	if( ret & 0x1000 ) {
		MM_DerefPhys(ret);
		MM_DerefPhys(ret+0x1000);
		ret = MM_AllocPhysRange(3, -1);
		if( ret & 0x1000 ) {
			MM_DerefPhys(ret);
			ret += 0x1000;
		}
		else {
			MM_DerefPhys(ret + 0x2000);
		}
	}
	return ret;
}

void MM_int_CloneTable(Uint32 *DestEnt, int Table)
{
	tPAddr	table;
	Uint32	*tmp_map;
	Uint32	*cur = (void*)MM_TABLE0USER;
//	Uint32	*cur = &FRACTAL(MM_TABLE1USER,0);
	 int	i;
	
	table = MM_AllocPhys();
	if(!table)	return ;
	
	tmp_map = (void*)MM_MapTemp(table);
	
	for( i = 0; i < 1024; i ++ )
	{
		switch(cur[i] & 3)
		{
		case 0:	tmp_map[i] = 0;	break;
		case 1:
			tmp_map[i] = 0;
			Log_Error("MMVirt", "TODO: Support large pages in MM_int_CloneTable");
			// Large page?
			break;
		case 2:
		case 3:
			// Small page
			// - If full RW
			if( (cur[Table*256] & 0x230) == 0x030 )
				cur[Table*256+i] |= 0x200;	// Set to full RO (Full RO=COW, User RO = RO)
			tmp_map[i] = cur[Table*256+i];
			break;
		}
	}

	DestEnt[0] = table + 0*0x400 + 1;
	DestEnt[1] = table + 1*0x400 + 1;
	DestEnt[2] = table + 2*0x400 + 1;
	DestEnt[3] = table + 3*0x400 + 1;
}

tPAddr MM_Clone(void)
{
	tPAddr	ret;
	Uint32	*new_lvl1_1, *new_lvl1_2, *cur;
	Uint32	*tmp_map;
	 int	i;
	
	ret = MM_AllocateRootTable();

	cur = (void*)MM_TABLE0USER;
	new_lvl1_1 = (void*)MM_MapTemp(ret);
	new_lvl1_2 = (void*)MM_MapTemp(ret+0x1000);
	tmp_map = new_lvl1_1;
	new_lvl1_1[0] = 0x8202;	// Section mapping the first meg for exception vectors (K-RO)
	for( i = 1; i < 0x800-4; i ++ )
	{
//		Log("i = %i", i);
		if( i == 0x400 ) {
			tmp_map = &new_lvl1_2[-0x400];
			Log("tmp_map = %p", tmp_map);
		}
		switch( cur[i] & 3 )
		{
		case 0:	tmp_map[i] = 0;	break;
		case 1:
			MM_int_CloneTable(&tmp_map[i], i);
			i += 3;	// Tables are alocated in blocks of 4
			break;
		case 2:
		case 3:
			Log_Error("MMVirt", "TODO: Support Sections/Supersections in MM_Clone (i=%i)", i);
			tmp_map[i] = 0;
			break;
		}
	}

	// Allocate Fractal table
	{
		 int	j, num;
		tPAddr	tmp = MM_AllocPhys();
		Uint32	*table = (void*)MM_MapTemp(tmp);
		Uint32	sp;
		register Uint32 __SP asm("sp");
		// Map table to last 4MiB of user space
		tmp_map[i+0] = tmp + 0*0x400 + 1;
		tmp_map[i+1] = tmp + 1*0x400 + 1;
		tmp_map[i+2] = tmp + 2*0x400 + 1;
		tmp_map[i+3] = tmp + 3*0x400 + 1;
		for( j = 0; j < 256; j ++ ) {
			table[j] = new_lvl1_1[j*4] & PADDR_MASK_LVL1;// 0xFFFFFC00;
			table[j] |= 0x10|3;	// Kernel Only, Small table, XN
		}
		for(      ; j < 512; j ++ ) {
			table[j] = new_lvl1_2[(j-256)*4] & PADDR_MASK_LVL1;// 0xFFFFFC00;
			table[j] |= 0x10|3;	// Kernel Only, Small table, XN
		}
		for(      ; j < 1024; j ++ )
			table[j] = 0;
		
		// Get kernel stack bottom
		sp = __SP;
		sp &= ~(MM_KSTACK_SIZE-1);
		j = (sp / 0x1000) % 1024;
		num = MM_KSTACK_SIZE/0x1000;
		Log("sp = %p, j = %i", sp, j);
		
		// Copy stack pages
		for(; num--; j ++, sp += 0x1000)
		{
			tVAddr	page = MM_AllocPhys();
			void	*tmp_page;
			table[j] = page | 0x13;
			tmp_page = (void*)MM_MapTemp(page);
			memcpy(tmp_page, (void*)sp, 0x1000);
			MM_FreeTemp( (tVAddr)tmp_page );
		}
		
		MM_FreeTemp( (tVAddr)table );
	}

	tmp_map = &tmp_map[0x400];
	MM_FreeTemp( (tVAddr)tmp_map );

	Log("Table dump");
	MM_DumpTables(0, -1);

	return ret;
}

tPAddr MM_ClearUser(void)
{
	// TODO: Implement ClearUser
	return 0;
}

tVAddr MM_MapTemp(tPAddr PAddr)
{
	tVAddr	ret;
	tMM_PageInfo	pi;
	
	for( ret = MM_TMPMAP_BASE; ret < MM_TMPMAP_END - PAGE_SIZE; ret += PAGE_SIZE )
	{
		if( MM_int_GetPageInfo(ret, &pi) == 0 )
			continue;
	
		MM_Map(ret, PAddr);
		
		return ret;
	}
	Log_Warning("MMVirt", "MM_MapTemp: All slots taken");
	return 0;
}

void MM_FreeTemp(tVAddr VAddr)
{
	// TODO: Implement FreeTemp
	if( VAddr < MM_TMPMAP_BASE || VAddr >= MM_TMPMAP_END ) {
		Log_Warning("MMVirt", "MM_FreeTemp: Passed an addr not from MM_MapTemp (%p)", VAddr);
		return ;
	}
	
	MM_Deallocate(VAddr);
}

tVAddr MM_MapHWPages(tPAddr PAddr, Uint NPages)
{
	tVAddr	ret;
	 int	i;
	tMM_PageInfo	pi;

	ENTER("xPAddr iNPages", PAddr, NPages);

	// Scan for a location
	for( ret = MM_HWMAP_BASE; ret < MM_HWMAP_END - NPages * PAGE_SIZE; ret += PAGE_SIZE )
	{
//		LOG("checking %p", ret);
		// Check if there is `NPages` free pages
		for( i = 0; i < NPages; i ++ )
		{
			if( MM_int_GetPageInfo(ret + i*PAGE_SIZE, &pi) == 0 )
				break;
		}
		// Nope, jump to after the used page found and try again
//		LOG("i = %i, ==? %i", i, NPages);
		if( i != NPages ) {
			ret += i * PAGE_SIZE;
			continue ;
		}
	
		// Map the pages	
		for( i = 0; i < NPages; i ++ )
			MM_Map(ret+i*PAGE_SIZE, PAddr+i*PAddr);
		// and return
		LEAVE('p', ret);
		return ret;
	}
	Log_Warning("MMVirt", "MM_MapHWPages: No space for a %i page block", NPages);
	LEAVE('p', 0);
	return 0;
}

tVAddr MM_AllocDMA(int Pages, int MaxBits, tPAddr *PAddr)
{
	Log_Error("MMVirt", "TODO: Implement MM_AllocDMA");
	return 0;
}

void MM_UnmapHWPages(tVAddr Vaddr, Uint Number)
{
	Log_Error("MMVirt", "TODO: Implement MM_UnmapHWPages");
}

tVAddr MM_NewKStack(int bShared)
{
	tVAddr	min_addr, max_addr;
	tVAddr	addr, ofs;

	if( bShared ) {
		min_addr = MM_GLOBALSTACKS;
		max_addr = MM_GLOBALSTACKS_END;
	}
	else {
		min_addr = MM_KSTACK_BASE;
		max_addr = MM_KSTACK_END;
	}

	// Locate a free slot
	for( addr = min_addr; addr < max_addr; addr += MM_KSTACK_SIZE )
	{
		tMM_PageInfo	pi;
		if( MM_int_GetPageInfo(addr+MM_KSTACK_SIZE-PAGE_SIZE, &pi) )	break;
	}

	// Check for an error	
	if(addr >= max_addr) {
		return 0;
	}

	// 1 guard page
	for( ofs = PAGE_SIZE; ofs < MM_KSTACK_SIZE; ofs += PAGE_SIZE )
	{
		if( MM_Allocate(addr + ofs) == 0 )
		{
			while(ofs)
			{
				ofs -= PAGE_SIZE;
				MM_Deallocate(addr + ofs);
			}
			Log_Warning("MMVirt", "MM_NewKStack: Unable to allocate");
			return 0;
		}
	}
	return addr + ofs;
}

void MM_int_DumpTableEnt(tVAddr Start, size_t Len, tMM_PageInfo *Info)
{
	Log("%p => %8x - 0x%7x %i %x",
		Start, Info->PhysAddr-Len, Len,
		Info->Domain,
		Info->AP
		);
}

void MM_DumpTables(tVAddr Start, tVAddr End)
{
	tVAddr	range_start = 0, addr;
	tMM_PageInfo	pi, pi_old;
	 int	i = 0, inRange=0;
	
	pi_old.Size = 0;

	range_start = Start;
	for( addr = Start; i == 0 || (addr && addr < End); i = 1 )
	{
		int rv = MM_int_GetPageInfo(addr, &pi);
		if( rv
		 || pi.Size != pi_old.Size
		 || pi.Domain != pi_old.Domain
		 || pi.AP != pi_old.AP
		 || pi_old.PhysAddr != pi.PhysAddr )
		{
			if(inRange) {
				MM_int_DumpTableEnt(range_start, addr - range_start, &pi_old);
			}
			addr &= ~((1 << pi.Size)-1);
			range_start = addr;
		}
		
		pi_old = pi;
		pi_old.PhysAddr += 1 << pi_old.Size;
		addr += 1 << pi_old.Size;
		inRange = (rv == 0);
	}
	if(inRange)
		MM_int_DumpTableEnt(range_start, addr - range_start, &pi);
}

