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

#define TRACE_MAPS	0

#define AP_KRW_ONLY	1	// Kernel page
#define AP_KRO_ONLY	5	// Kernel RO page
#define AP_RW_BOTH	3	// Standard RW
#define AP_RO_BOTH	6	// COW Page
#define AP_RO_USER	2	// User RO Page
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
#define USRFRACTAL(addr)	(*((Uint32*)(0x7FDFF000) + ((addr)>>22)))
#define TLBIALL()	__asm__ __volatile__ ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0))
#define TLBIMVA(addr)	__asm__ __volatile__ ("mcr p15, 0, %0, c8, c7, 1" : : "r" (addr))

// === PROTOTYPES ===
void	MM_int_GetTables(tVAddr VAddr, Uint32 **Table0, Uint32 **Table1);
 int	MM_int_AllocateCoarse(tVAddr VAddr, int Domain);
 int	MM_int_SetPageInfo(tVAddr VAddr, tMM_PageInfo *pi);
 int	MM_int_GetPageInfo(tVAddr VAddr, tMM_PageInfo *pi);
tVAddr	MM_NewUserStack(void);
tPAddr	MM_AllocateZero(tVAddr VAddr);
tPAddr	MM_AllocateRootTable(void);
void	MM_int_CloneTable(Uint32 *DestEnt, int Table);
tPAddr	MM_Clone(void);
tVAddr	MM_NewKStack(int bGlobal);
void	MM_int_DumpTableEnt(tVAddr Start, size_t Len, tMM_PageInfo *Info);
//void	MM_DumpTables(tVAddr Start, tVAddr End);

// === GLOBALS ===
tPAddr	giMM_ZeroPage;

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

	if( VAddr < 0x80000000 ) {
//		Log("USRFRACTAL(%p) = %p", VAddr, &USRFRACTAL(VAddr));
		USRFRACTAL(VAddr) = paddr | 3;
	}
	else {
//		Log("FRACTAL(%p) = %p", VAddr, &FRACTAL(table1, VAddr));
		FRACTAL(table1, VAddr) = paddr | 3;
	}

	// TLBIALL 
	TLBIALL();	

	LEAVE('i', 0);
	return 0;
}	

int MM_int_SetPageInfo(tVAddr VAddr, tMM_PageInfo *pi)
{
	Uint32	*table0, *table1;
	Uint32	*desc;

	ENTER("pVAddr ppi", VAddr, pi);

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
			TLBIMVA(VAddr & 0xFFFFF000);
			LEAVE('i', 0);
			return 0;
		}
		else
		{
			// Large page
			// TODO: 
			Log_Warning("MMVirt", "TODO: Implement large pages in MM_int_SetPageInfo");
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
	case 0:
		break;
	case AP_KRW_ONLY:
		ret |= MM_PFLAG_KERNEL;
		break;
	case AP_KRO_ONLY:
		ret |= MM_PFLAG_KERNEL|MM_PFLAG_RO;
		break;
	case AP_RW_BOTH:
		break;
	case AP_RO_BOTH:
		ret |= MM_PFLAG_COW;
		break;
	case AP_RO_USER:
		ret |= MM_PFLAG_RO;
		break;
	}

	if( pi.bExecutable )	ret |= MM_PFLAG_EXEC;
	return ret;
}

void MM_SetFlags(tVAddr VAddr, Uint Flags, Uint Mask)
{
	tMM_PageInfo	pi;
	Uint	curFlags;
	
	if( MM_int_GetPageInfo(VAddr, &pi) )
		return ;
	
	curFlags = MM_GetPhysAddr(VAddr);
	if( (curFlags & Mask) == Flags )
		return ;
	curFlags &= ~Mask;
	curFlags |= Flags;

	if( curFlags & MM_PFLAG_COW )
		pi.AP = AP_RO_BOTH;
	else
	{
		switch(curFlags & (MM_PFLAG_KERNEL|MM_PFLAG_RO) )
		{
		case 0:
			pi.AP = AP_RW_BOTH;	break;
		case MM_PFLAG_KERNEL:
			pi.AP = AP_KRW_ONLY;	break;
		case MM_PFLAG_RO:
			pi.AP = AP_RO_USER;	break;
		case MM_PFLAG_KERNEL|MM_PFLAG_RO:
			pi.AP = AP_KRO_ONLY;	break;
		}
	}
	
	pi.bExecutable = !!(curFlags & MM_PFLAG_EXEC);

	MM_int_SetPageInfo(VAddr, &pi);
}

int MM_Map(tVAddr VAddr, tPAddr PAddr)
{
	tMM_PageInfo	pi = {0};
	#if TRACE_MAPS
	Log("MM_Map %P=>%p", PAddr, VAddr);
	#endif
	
	pi.PhysAddr = PAddr;
	pi.Size = 12;
	if(VAddr < USER_STACK_TOP)
		pi.AP = AP_RW_BOTH;
	else
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
	if(VAddr < USER_STACK_TOP)
		pi.AP = AP_RW_BOTH;
	else
		pi.AP = AP_KRW_ONLY;
	pi.bExecutable = 1;
	if( MM_int_SetPageInfo(VAddr, &pi) ) {
		MM_DerefPhys(pi.PhysAddr);
		LEAVE('i', 0);
		return 0;
	}
	LEAVE('x', pi.PhysAddr);
	return pi.PhysAddr;
}

tPAddr MM_AllocateZero(tVAddr VAddr)
{
	if( !giMM_ZeroPage ) {
		giMM_ZeroPage = MM_Allocate(VAddr);
		MM_RefPhys(giMM_ZeroPage);
		memset((void*)VAddr, 0, PAGE_SIZE);
	}
	else {
		MM_RefPhys(giMM_ZeroPage);
		MM_Map(VAddr, giMM_ZeroPage);
	}
	MM_SetFlags(VAddr, MM_PFLAG_COW, MM_PFLAG_COW);
	return giMM_ZeroPage;
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
//			Log("MM_AllocateRootTable: Second try not aligned, %P", ret);
		}
		else {
			MM_DerefPhys(ret + 0x2000);
//			Log("MM_AllocateRootTable: Second try aligned, %P", ret);
		}
	}
//	else
//		Log("MM_AllocateRootTable: Got it in one, %P", ret);
	return ret;
}

void MM_int_CloneTable(Uint32 *DestEnt, int Table)
{
	tPAddr	table;
	Uint32	*tmp_map;
	Uint32	*cur = (void*)MM_TABLE1USER;
//	Uint32	*cur = &FRACTAL(MM_TABLE1USER,0);
	 int	i;
	
	table = MM_AllocPhys();
	if(!table)	return ;
	
	tmp_map = (void*)MM_MapTemp(table);
	
	for( i = 0; i < 1024; i ++ )
	{
//		Log_Debug("MMVirt", "cur[%i] (%p) = %x", Table*256+i, &cur[Table*256+i], cur[Table*256+i]);
		switch(cur[Table*256+i] & 3)
		{
		case 0:	tmp_map[i] = 0;	break;
		case 1:
			tmp_map[i] = 0;
			Log_Error("MMVirt", "TODO: Support large pages in MM_int_CloneTable (%p)", (Table*256+i)*0x1000);
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

//	MM_DumpTables(0, KERNEL_BASE);
	
	ret = MM_AllocateRootTable();

	cur = (void*)MM_TABLE0USER;
	new_lvl1_1 = (void*)MM_MapTemp(ret);
	new_lvl1_2 = (void*)MM_MapTemp(ret+0x1000);
	tmp_map = new_lvl1_1;
	for( i = 0; i < 0x800-4; i ++ )
	{
		// HACK! Ignore the original identity mapping
		if( i == 0 && Threads_GetTID() == 0 ) {
			tmp_map[0] = 0;
			continue;
		}
		if( i == 0x400 )
			tmp_map = &new_lvl1_2[-0x400];
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
		new_lvl1_2[0x3FC] = tmp + 0*0x400 + 1;
		new_lvl1_2[0x3FD] = tmp + 1*0x400 + 1;
		new_lvl1_2[0x3FE] = tmp + 2*0x400 + 1;
		new_lvl1_2[0x3FF] = tmp + 3*0x400 + 1;
		
		tmp_map = new_lvl1_1;
		for( j = 0; j < 512; j ++ )
		{
			if( j == 256 )
				tmp_map = &new_lvl1_2[-0x400];
			if( (tmp_map[j*4] & 3) == 1 )
			{
				table[j] = tmp_map[j*4] & PADDR_MASK_LVL1;// 0xFFFFFC00;
				table[j] |= 0x813;	// nG, Kernel Only, Small page, XN
			}
			else
				table[j] = 0;
		}
		// Fractal
		table[j++] = (ret + 0x0000) | 0x813;
		table[j++] = (ret + 0x1000) | 0x813;
		// Nuke the rest
		for(      ; j < 1024; j ++ )
			table[j] = 0;
		
		// Get kernel stack bottom
		sp = __SP & ~(MM_KSTACK_SIZE-1);
		j = (sp / 0x1000) % 1024;
		num = MM_KSTACK_SIZE/0x1000;
		
		// Copy stack pages
		for(; num--; j ++, sp += 0x1000)
		{
			tVAddr	page;
			void	*tmp_page;
			
			page = MM_AllocPhys();
			table[j] = page | 0x813;

			tmp_page = (void*)MM_MapTemp(page);
			memcpy(tmp_page, (void*)sp, 0x1000);
			MM_FreeTemp( (tVAddr) tmp_page );
		}
	
		MM_FreeTemp( (tVAddr)table );
	}

	MM_FreeTemp( (tVAddr)new_lvl1_1 );
	MM_FreeTemp( (tVAddr)new_lvl1_2 );

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

//		Log("MapTemp %P at %p", PAddr, ret);	
		MM_RefPhys(PAddr);	// Counter the MM_Deallocate in FreeTemp
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
			MM_Map(ret+i*PAGE_SIZE, PAddr+i*PAGE_SIZE);
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
	tPAddr	phys;
	tVAddr	ret;

	phys = MM_AllocPhysRange(Pages, MaxBits);
	if(!phys) {
		Log_Warning("MMVirt", "No space left for a %i page block (MM_AllocDMA)", Pages);
		return 0;
	}
	
	ret = MM_MapHWPages(phys, Pages);
	*PAddr = phys;

	return ret;
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

tVAddr MM_NewUserStack(void)
{
	tVAddr	addr, ofs;

	addr = USER_STACK_TOP - USER_STACK_SIZE;
	if( MM_GetPhysAddr(addr + PAGE_SIZE) ) {
		Log_Error("MMVirt", "Unable to create initial user stack, addr %p taken",
			addr + PAGE_SIZE
			);
		return 0;
	}

	// 1 guard page
	for( ofs = PAGE_SIZE; ofs < USER_STACK_SIZE; ofs += PAGE_SIZE )
	{
		tPAddr	rv;
		if(ofs >= USER_STACK_SIZE - USER_STACK_COMM)
			rv = MM_Allocate(addr + ofs);
		else
			rv = MM_AllocateZero(addr + ofs);
		if(rv == 0)
		{
			while(ofs)
			{
				ofs -= PAGE_SIZE;
				MM_Deallocate(addr + ofs);
			}
			Log_Warning("MMVirt", "MM_NewUserStack: Unable to allocate");
			return 0;
		}
		MM_SetFlags(addr+ofs, 0, MM_PFLAG_KERNEL);
	}
	Log("Return %p", addr + ofs);
	MM_DumpTables(0, 0x80000000);
	return addr + ofs;
}

void MM_int_DumpTableEnt(tVAddr Start, size_t Len, tMM_PageInfo *Info)
{
	if( giMM_ZeroPage && Info->PhysAddr == giMM_ZeroPage )
	{
		Log("%p => %8s - 0x%7x %i %x",
			Start, "ZERO", Len,
			Info->Domain, Info->AP
			);
	}
	else
	{
		Log("%p => %8x - 0x%7x %i %x",
			Start, Info->PhysAddr-Len, Len,
			Info->Domain, Info->AP
			);
	}
}

void MM_DumpTables(tVAddr Start, tVAddr End)
{
	tVAddr	range_start = 0, addr;
	tMM_PageInfo	pi, pi_old;
	 int	i = 0, inRange=0;
	
	pi_old.Size = 0;

	Log("Page Table Dump:");
	range_start = Start;
	for( addr = Start; i == 0 || (addr && addr < End); i = 1 )
	{
//		Log("addr = %p", addr);
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
		// Handle the zero page
		if( !giMM_ZeroPage || pi_old.Size != 12 || pi_old.PhysAddr != giMM_ZeroPage )
			pi_old.PhysAddr += 1 << pi_old.Size;
		addr += 1 << pi_old.Size;
		inRange = (rv == 0);
	}
	if(inRange)
		MM_int_DumpTableEnt(range_start, addr - range_start, &pi);
	Log("Done");
}

