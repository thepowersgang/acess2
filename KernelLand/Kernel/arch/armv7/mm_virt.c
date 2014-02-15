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
#define TRACE_COW	0

#define AP_KRW_ONLY	1	// Kernel page
#define AP_KRO_ONLY	5	// Kernel RO page
#define AP_RW_BOTH	3	// Standard RW
#define AP_RO_BOTH	7	// COW Page
#define AP_RO_USER	2	// User RO Page
#define PADDR_MASK_LVL1	0xFFFFFC00

const char * const caAPValueNames[] = {
	"AP_NOACCESS", "AP_KRW_ONLY",
	"AP_RO_USER", "AP_RW_BOTH",
	"AP_???_4", "AP_KRO_ONLY",
	"AP_???_6", "AP_RO_BOTH"
};

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
#define TLBIMVA(addr)	__asm__ __volatile__ ("mcr p15, 0, %0, c8, c7, 1;dsb;isb" : : "r" (((addr)&~0xFFF)|1):"memory")
#define DCCMVAC(addr)	__asm__ __volatile__ ("mcr p15, 0, %0, c7, c10, 1" : : "r" ((addr)&~0xFFF))

// === PROTOTYPES ===
void	MM_int_GetTables(tVAddr VAddr, Uint32 **Table0, Uint32 **Table1);
 int	MM_int_AllocateCoarse(tVAddr VAddr, int Domain);
 int	MM_int_SetPageInfo(tVAddr VAddr, tMM_PageInfo *pi);
 int	MM_int_GetPageInfo(tVAddr VAddr, tMM_PageInfo *pi);
tVAddr	MM_NewUserStack(void);
//tPAddr	MM_AllocateZero(volatile void *VAddr);
tPAddr	MM_AllocateRootTable(void);
void	MM_int_CloneTable(Uint32 *DestEnt, int Table);
tPAddr	MM_Clone(int ClearUser);
tVAddr	MM_NewKStack(int bGlobal);
void	MM_int_DumpTableEnt(tVAddr Start, size_t Len, tMM_PageInfo *Info);
//void	MM_DumpTables(tVAddr Start, tVAddr End);
void	MM_PageFault(Uint32 PC, Uint32 Addr, Uint32 DFSR, int bPrefetch, Uint32 SystemLR, Uint32 UserLR);

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
		USRFRACTAL(VAddr) = paddr | 0x13;
	}
	else {
		FRACTAL(table1, VAddr) = paddr | 0x13;
	}

	// TLBIALL 
	TLBIALL();
	
	memset( (void*)&table1[ (VAddr >> 12) & ~(1024-1) ], 0, 0x1000 );

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
				TLBIMVA( VAddr );
				DCCMVAC( (tVAddr) desc );
//				#warning "HACK: TLBIALL"
//				TLBIALL();				
				LEAVE('i', 0);
				return 0;
			}

			*desc = (pi->PhysAddr & 0xFFFFF000) | 2;
			if(!pi->bExecutable)	*desc |= 1;	// XN
			if(!pi->bGlobal)	*desc |= 1 << 11;	// nG
			if( pi->bShared)	*desc |= 1 << 10;	// S
			*desc |= (pi->AP & 3) << 4;	// AP
			*desc |= ((pi->AP >> 2) & 1) << 9;	// APX
			TLBIMVA( VAddr );	
//			#warning "HACK: TLBIALL"
//			TLBIALL();
			DCCMVAC( (tVAddr) desc );
			LEAVE('i', 0);
			return 0;
		}
		else
		{
			// Large page
			Log_Warning("MMVirt", "TODO: Implement large pages in MM_int_SetPageInfo");
		}
		break;
	case 20:	// Section or unmapped
		Log_Warning("MMVirt", "TODO: Implement sections in MM_int_SetPageInfo");
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
			}
			else {
				// Apply
				*desc = pi->PhysAddr & 0xFF000000;
//				*desc |= ((pi->PhysAddr >> 32) & 0xF) << 20;
//				*desc |= ((pi->PhysAddr >> 36) & 0x7) << 5;
				*desc |= 2 | (1 << 18);
			}
			// TODO: Apply to all entries
			Log_Warning("MMVirt", "TODO: Apply changes to all entries of supersections");
			LEAVE('i', 0);
			return 0;
		}
		// TODO: What here?
		Log_Warning("MMVirt", "TODO: 24-bit not on supersection?");
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
tPAddr MM_GetPhysAddr(volatile const void *Ptr)
{
	tMM_PageInfo	pi;
	if( MM_int_GetPageInfo((tVAddr)Ptr, &pi) )
		return 0;
	return pi.PhysAddr | ((tVAddr)Ptr & ((1 << pi.Size)-1));
}

Uint MM_GetFlags(const volatile void *VAddr)
{
	tMM_PageInfo	pi;
	 int	ret;

	if( MM_int_GetPageInfo((tVAddr)VAddr, &pi) )
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

void MM_SetFlags(volatile void *VAddr, Uint Flags, Uint Mask)
{
	tMM_PageInfo	pi;
	Uint	curFlags;
	
	if( MM_int_GetPageInfo((tVAddr)VAddr, &pi) )
		return ;
	
	curFlags = MM_GetFlags(VAddr);
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

	MM_int_SetPageInfo((tVAddr)VAddr, &pi);
}

int MM_IsValidBuffer(tVAddr Addr, size_t Size)
{
	tMM_PageInfo	pi;
	 int	bUser = 0;
	
	Size += Addr & (PAGE_SIZE-1);
	Addr &= ~(PAGE_SIZE-1);

	if( MM_int_GetPageInfo((tVAddr)Addr, &pi) )	return 0;
	Addr += PAGE_SIZE;

	if(pi.AP != AP_KRW_ONLY && pi.AP != AP_KRO_ONLY)
		bUser = 1;

	while( Size >= PAGE_SIZE )
	{
		if( MM_int_GetPageInfo(Addr, &pi) )
			return 0;
		if(bUser && (pi.AP == AP_KRW_ONLY || pi.AP == AP_KRO_ONLY))
			return 0;
		Addr += PAGE_SIZE;
		Size -= PAGE_SIZE;
	}
	
	return 1;
}

int MM_Map(volatile void *VAddr, tPAddr PAddr)
{
	tMM_PageInfo	pi = {0};
	#if TRACE_MAPS
	Log("MM_Map %P=>%p", PAddr, VAddr);
	#endif
	
	// TODO: Double check that an address isn't being clobbered
	
	pi.PhysAddr = PAddr;
	pi.Size = 12;
	pi.AP = ( (tVAddr)VAddr < USER_STACK_TOP ? AP_RW_BOTH : AP_KRW_ONLY );
	pi.bExecutable = 1;
	if( MM_int_SetPageInfo( (tVAddr)VAddr, &pi) ) {
//		MM_DerefPhys(pi.PhysAddr);
		return 0;
	}
	return pi.PhysAddr;
}

tPAddr MM_Allocate(volatile void *VAddr)
{
	tMM_PageInfo	pi = {0};
	
	ENTER("pVAddr", VAddr);

	pi.PhysAddr = MM_AllocPhys();
	if( pi.PhysAddr == 0 )	LEAVE_RET('i', 0);
	pi.Size = 12;
	pi.AP = ( (tVAddr)VAddr < USER_STACK_TOP ? AP_RW_BOTH : AP_KRW_ONLY );
	pi.bExecutable = 0;
	if( MM_int_SetPageInfo( (tVAddr)VAddr, &pi ) )
	{
		MM_DerefPhys(pi.PhysAddr);
		LEAVE('i', 0);
		return 0;
	}
	LEAVE('x', pi.PhysAddr);
	return pi.PhysAddr;
}

void MM_AllocateZero(volatile void *VAddr)
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
}

void MM_Deallocate(volatile void *VAddr)
{
	tMM_PageInfo	pi;
	
	if( MM_int_GetPageInfo((tVAddr)VAddr, &pi) )	return ;
	if( pi.PhysAddr == 0 )	return;
	MM_DerefPhys(pi.PhysAddr);
	
	pi.PhysAddr = 0;
	pi.AP = 0;
	pi.bExecutable = 0;
	MM_int_SetPageInfo((tVAddr)VAddr, &pi);
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

	cur += 256*Table;
	
	tmp_map = MM_MapTemp(table);
	
	for( i = 0; i < 1024; i ++ )
	{
//		Log_Debug("MMVirt", "cur[%i] (%p) = %x", Table*256+i, &cur[Table*256+i], cur[Table*256+i]);
		switch(cur[i] & 3)
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
//			Debug("%p cur[%i] & 0x230 = 0x%x", Table*256*0x1000, i, cur[i] & 0x230);
			if( (cur[i] & 0x230) == 0x010 )
			{
				void	*dst, *src;
				tPAddr	newpage;
				newpage = MM_AllocPhys();
				src = (void*)( (Table*256+i)*0x1000 );
				dst = MM_MapTemp(newpage);
//				Debug("Taking a copy of kernel page %p (%P)", src, cur[i] & ~0xFFF);
				memcpy(dst, src, PAGE_SIZE);
				MM_FreeTemp( dst );
				tmp_map[i] = newpage | (cur[i] & 0xFFF);
			}
			else
			{
				if( (cur[i] & 0x230) == 0x030 )
					cur[i] |= 0x200;	// Set to full RO (Full RO=COW, User RO = RO)
				tmp_map[i] = cur[i];
				MM_RefPhys( tmp_map[i] & ~0xFFF );
			}
			break;
		}
	}
	MM_FreeTemp( tmp_map );

	DestEnt[0] = table + 0*0x400 + 1;
	DestEnt[1] = table + 1*0x400 + 1;
	DestEnt[2] = table + 2*0x400 + 1;
	DestEnt[3] = table + 3*0x400 + 1;
}

tPAddr MM_Clone(int EmptyUser)
{
	Uint32	*tmp_map;

//	MM_DumpTables(0, KERNEL_BASE);
	
	tPAddr	ret = MM_AllocateRootTable();

	Uint32	*new_lvl1_1 = MM_MapTemp(ret);
	Uint32	*new_lvl1_2 = MM_MapTemp(ret+0x1000);
	
	if( !EmptyUser )
	{
		Uint32 *cur = (void*)MM_TABLE0USER;
		tmp_map = new_lvl1_1;
		for( int i = 0; i < 0x800-4; i ++ )
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
	}

	// Allocate Fractal table
	{
		tPAddr	tmp = MM_AllocPhys();
		Uint32	*table = MM_MapTemp(tmp);

		// Map table to last 4MiB of user space
		new_lvl1_2[0x3FC] = tmp + 0*0x400 + 1;
		new_lvl1_2[0x3FD] = tmp + 1*0x400 + 1;
		new_lvl1_2[0x3FE] = tmp + 2*0x400 + 1;
		new_lvl1_2[0x3FF] = tmp + 3*0x400 + 1;
		
		tmp_map = new_lvl1_1;
		 int	j = 0;
		for( ; j < 512; j ++ )
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
		// Clear the rest of the table
		for( ; j < 1024; j ++ )
			table[j] = 0;
		
		// Get kernel stack bottom
		register Uint32 __SP asm("sp");
		Uint32	sp = __SP & ~(MM_KSTACK_SIZE-1);
		j = (sp / 0x1000) % 1024;

		// Copy stack pages
		for(int num = MM_KSTACK_SIZE/PAGE_SIZE; num--; j ++, sp += PAGE_SIZE)
		{
			tVAddr	page;
			void	*tmp_page;
			
			page = MM_AllocPhys();
//			Log("page = %P", page);
			table[j] = page | 0x813;

			tmp_page = MM_MapTemp(page);
			memcpy(tmp_page, (void*)sp, 0x1000);
			MM_FreeTemp( tmp_page );
		}

		MM_FreeTemp( table );
	}

	MM_FreeTemp( new_lvl1_1 );
	MM_FreeTemp( new_lvl1_2 );

//	Log("MM_Clone: ret = %P", ret);

	return ret;
}

void MM_ClearUser(void)
{
	 int	i, j;
	const int	user_table_count = USER_STACK_TOP / (256*0x1000);
	Uint32	*cur = (void*)MM_TABLE0USER;
	Uint32	*tab;
	
//	MM_DumpTables(0, 0x80000000);

//	Log("user_table_count = %i (as opposed to %i)", user_table_count, 0x800-4);

	for( i = 0; i < user_table_count; i ++ )
	{
		switch( cur[i] & 3 )
		{
		case 0:	break;	// Already unmapped
		case 1:	// Sub pages
			tab = (void*)(MM_TABLE1USER + i*256*sizeof(Uint32));
			for( j = 0; j < 1024; j ++ )
			{
				switch( tab[j] & 3 )
				{
				case 0:	break;	// Unmapped
				case 1:
					Log_Error("MMVirt", "TODO: Support large pages in MM_ClearUser");
					break;
				case 2:
				case 3:
					MM_DerefPhys( tab[j] & ~(PAGE_SIZE-1) );
					break;
				}
			}
			MM_DerefPhys( cur[i] & ~(PAGE_SIZE-1) );
			cur[i+0] = 0;
			cur[i+1] = 0;
			cur[i+2] = 0;
			i += 3;
			break;
		case 2:
		case 3:
			Log_Error("MMVirt", "TODO: Implement sections/supersections in MM_ClearUser");
			break;
		}
		cur[i] = 0;
	}
	
	// Final block of 4 tables are KStack
	i = 0x800 - 4;
	
	// Clear out unused stacks
	{
		register Uint32 __SP asm("sp");
		 int	cur_stack_base = ((__SP & ~(MM_KSTACK_SIZE-1)) / PAGE_SIZE) % 1024;

		tab = (void*)(MM_TABLE1USER + i*256*sizeof(Uint32));
		
		// First 512 is the Table1 mapping + 2 for Table0 mapping
		for( j = 512+2; j < 1024; j ++ )
		{
			// Skip current stack
			if( j == cur_stack_base ) {
				j += (MM_KSTACK_SIZE / PAGE_SIZE) - 1;
				continue ;
			}
			if( !(tab[j] & 3) )	continue;
			ASSERT( (tab[j] & 3) == 2 );
			MM_DerefPhys( tab[j] & ~(PAGE_SIZE) );
			tab[j] = 0;
		}
	}
	

//	MM_DumpTables(0, 0x80000000);
}

void *MM_MapTemp(tPAddr PAddr)
{
	for( tVAddr ret = MM_TMPMAP_BASE; ret < MM_TMPMAP_END - PAGE_SIZE; ret += PAGE_SIZE )
	{
		tMM_PageInfo	pi;

		if( MM_int_GetPageInfo(ret, &pi) == 0 )
			continue;

//		Log("MapTemp %P at %p by %p", PAddr, ret, __builtin_return_address(0));
		MM_RefPhys(PAddr);	// Counter the MM_Deallocate in FreeTemp
		MM_Map( (void*)ret, PAddr );
		
		return (void*)ret;
	}
	Log_Warning("MMVirt", "MM_MapTemp: All slots taken");
	return 0;
}

void MM_FreeTemp(void *Ptr)
{
	tVAddr	VAddr = (tVAddr)Ptr;
	if( VAddr < MM_TMPMAP_BASE || VAddr >= MM_TMPMAP_END ) {
		Log_Warning("MMVirt", "MM_FreeTemp: Passed an addr not from MM_MapTemp (%p)", VAddr);
		return ;
	}
	
	MM_Deallocate(Ptr);
}

void *MM_MapHWPages(tPAddr PAddr, Uint NPages)
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
			MM_Map( (tPage*)ret + i, PAddr+i*PAGE_SIZE);
		// and return
		LEAVE('p', ret);
		return (void*)ret;
	}
	Log_Warning("MMVirt", "MM_MapHWPages: No space for a %i page block", NPages);
	LEAVE('p', 0);
	return 0;
}

void *MM_AllocDMA(int Pages, int MaxBits, tPAddr *PAddr)
{
	tPAddr	phys;
	void	*ret;

	phys = MM_AllocPhysRange(Pages, MaxBits);
	if(!phys) {
		Log_Warning("MMVirt", "No space left for a %i page block (MM_AllocDMA)", Pages);
		return 0;
	}
	
	ret = MM_MapHWPages(phys, Pages);
	if( !ret ) {
		MM_DerefPhys(phys);
		return NULL;
	}
	if( PAddr )
		*PAddr = phys;

	return ret;
}

void MM_UnmapHWPages(volatile void *VAddr, Uint Number)
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
	tPage	*pageptr = (void*)(addr + PAGE_SIZE);
	for( ofs = PAGE_SIZE; ofs < MM_KSTACK_SIZE; ofs += PAGE_SIZE )
	{
		if( MM_Allocate( pageptr ) == 0 )
		{
			while(ofs)
			{
				ofs -= PAGE_SIZE;
				MM_Deallocate( pageptr-- );
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
	if( MM_GetPhysAddr( (void*)(addr + PAGE_SIZE) ) ) {
		Log_Error("MMVirt", "Unable to create initial user stack, addr %p taken",
			addr + PAGE_SIZE
			);
		MM_DumpTables(0,KERNEL_BASE);
		return 0;
	}

	// 1 guard page
	tPage	*pageptr = (void*)addr;
	for( ofs = PAGE_SIZE; ofs < USER_STACK_SIZE; ofs += PAGE_SIZE, pageptr ++ )
	{
		if(ofs >= USER_STACK_SIZE - USER_STACK_COMM) {
			tPAddr	rv = MM_Allocate(pageptr);
			if(rv == 0)
			{
				while(ofs)
				{
					ofs -= PAGE_SIZE;
					MM_Deallocate(pageptr --);
				}
				Log_Warning("MMVirt", "MM_NewUserStack: Unable to allocate");
				return 0;
			}
		}
		else {
			MM_AllocateZero(pageptr);
		}
		MM_SetFlags(pageptr, 0, MM_PFLAG_KERNEL);
	}
	Log("Return %p", addr + ofs);
//	MM_DumpTables(0, 0x80000000);
	return addr + ofs;
}

void MM_int_DumpTableEnt(tVAddr Start, size_t Len, tMM_PageInfo *Info)
{
	if( giMM_ZeroPage && Info->PhysAddr == giMM_ZeroPage )
	{
		Debug("0x%08x => %8s - 0x%7x D%i %x %s %s",
			Start, "ZERO", Len,
			Info->Domain, Info->AP,
			Info->bExecutable ? " X" : "nX",
			Info->bGlobal ? " G" : "nG"
			);
	}
	else
	{
		void	*node;
		MM_GetPageNode(Info->PhysAddr, &node);
		Debug("0x%08x => %8x - 0x%7x D%i %x %s %s %p",
			Start, Info->PhysAddr-Len, Len,
			Info->Domain, Info->AP,
			Info->bExecutable ? " X" : "nX",
			Info->bGlobal ? " G" : "nG",
			node
			);
	}
}

void MM_DumpTables(tVAddr Start, tVAddr End)
{
	tVAddr	range_start = 0, addr;
	tMM_PageInfo	pi, pi_old;
	 int	i = 0, inRange=0;
	
	memset(&pi_old, 0, sizeof(pi_old));

	Debug("Page Table Dump (%p to %p):", Start, End);
	range_start = Start;
	for( addr = Start; i == 0 || (addr && addr < End); i = 1 )
	{
		 int	rv;
//		Log("addr = %p", addr);
		rv = MM_int_GetPageInfo(addr, &pi);
		if( rv
		 || pi.Size != pi_old.Size
		 || pi.Domain != pi_old.Domain
		 || pi.AP != pi_old.AP
		 || pi.bGlobal != pi_old.bGlobal
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
	Debug("Done");
}

// NOTE: Runs in abort context, not much difference, just a smaller stack
void MM_PageFault(Uint32 PC, Uint32 Addr, Uint32 DFSR, int bPrefetch, Uint32 SystemLR, Uint32 UserLR)
{
	 int	rv;
	tMM_PageInfo	pi;
	
	rv = MM_int_GetPageInfo(Addr, &pi);
	
	// Check for COW
	if( rv == 0 &&  pi.AP == AP_RO_BOTH )
	{
		pi.AP = AP_RW_BOTH;
		if( giMM_ZeroPage && pi.PhysAddr == giMM_ZeroPage )
		{
			tPAddr	newpage;
			newpage = MM_AllocPhys();
			if( !newpage ) {
				Log_Error("MMVirt", "Unable to allocate new page for COW of ZERO");
				for(;;);
			}
			
			#if TRACE_COW
			Log_Notice("MMVirt", "COW %p caused by %p, ZERO duped to %P (RefCnt(%i)--)", Addr, PC,
				newpage, MM_GetRefCount(pi.PhysAddr));
			#endif

			MM_DerefPhys(pi.PhysAddr);
			pi.PhysAddr = newpage;
			pi.AP = AP_RW_BOTH;
			MM_int_SetPageInfo(Addr, &pi);
			
			memset( (void*)(Addr & ~(PAGE_SIZE-1)), 0, PAGE_SIZE );

			return ;
		}
		else if( MM_GetRefCount(pi.PhysAddr) > 1 )
		{
			// Duplicate the page
			tPAddr	newpage;
			void	*dst, *src;
			
			newpage = MM_AllocPhys();
			if(!newpage) {
				Log_Error("MMVirt", "Unable to allocate new page for COW");
				for(;;);
			}
			dst = MM_MapTemp(newpage);
			src = (void*)(Addr & ~(PAGE_SIZE-1));
			memcpy( dst, src, PAGE_SIZE );
			MM_FreeTemp( dst );
			
			#if TRACE_COW
			Log_Notice("MMVirt", "COW %p caused by %p, %P duped to %P (RefCnt(%i)--)", Addr, PC,
				pi.PhysAddr, newpage, MM_GetRefCount(pi.PhysAddr));
			#endif

			MM_DerefPhys(pi.PhysAddr);
			pi.PhysAddr = newpage;
		}
		#if TRACE_COW
		else {
			Log_Notice("MMVirt", "COW %p caused by %p, took last reference to %P",
				Addr, PC, pi.PhysAddr);
		}
		#endif
		// Unset COW
		pi.AP = AP_RW_BOTH;
		MM_int_SetPageInfo(Addr, &pi);
		return ;
	}
	

	Log_Error("MMVirt", "Code at %p accessed %p (DFSR = 0x%x)%s", PC, Addr, DFSR,
		(bPrefetch ? " - Prefetch" : "")
		);
	Log_Error("MMVirt", "- User LR = 0x%x, System LR = 0x%x", UserLR, SystemLR);
	const char * const dfsr_errors[] = {
		/* 00000 */ "-", "Alignment Fault",
		/* 00010 */ "Debug event", "Access Flag (Section)",
		/* 00100 */ "Instr Cache Maint", "Translation (Section)",
		/* 00110 */ "Access Flag (Page)", "Translation (Page)",
		/* 01000 */ "Sync. External abort", "Domain (Section)",
		/* 01010 */ "-", "Domain (Page)",
		/* 01100 */ "Table Walk sync ext (lvl 1)", "Permission (Section)",
		/* 01110 */ "Table Walk sync ext (lvl 2)", "Permission (Page)",
		// 0b10000
		/* 10000 */ "-", "-",
		/* 10010 */ "-", "-",
		/* 10100 */ "IMPL (Lockdown)", "-",
		/* 10110 */ "Async. Extern. Abort", "-",
		/* 11000 */ "Mem. access async pairity error", "Mem. access async pairity error",
		/* 11010 */ "IMPL (Coprocessor abort)", "-",
		/* 11100 */ "Table Walk Sync parity (lvl 1)", "-",
		/* 11110 */ "Table Walk Sync parity (lvl 2)", "-"
		};
	 int	errcode = (DFSR & 0xF) | (((DFSR >> 10) & 1) << 4);
	Log_Error("MMVirt", "- Errcode 0b%05b", errcode);
	Log_Error("MMVirt", "- Dom %i %s %s",
		(DFSR >> 4) & 0xF, (DFSR & 0x800 ? "Write": "Read"),
		dfsr_errors[errcode]
		);
	Log_Error("MMVirt", "- AP=%i(%s) %s", pi.AP, caAPValueNames[pi.AP], pi.bExecutable ? " Executable":"");
	if( Addr < 0x80000000 )
		MM_DumpTables(0, 0x80000000);
	else
		MM_DumpTables(0x80000000, -1);
	for(;;);
}

