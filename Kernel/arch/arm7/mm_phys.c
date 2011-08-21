/*
 * Acess2
 *
 * ARM7 Physical Memory Manager
 * arch/arm7/mm_phys.c
 */
#include <acess.h>
#include <mm_virt.h>

#define MM_NUM_RANGES	1	// Single range
#define MM_RANGE_MAX	0

#define NUM_STATIC_ALLOC	4

char	gStaticAllocPages[NUM_STATIC_ALLOC][PAGE_SIZE] __attribute__ ((section(".padata")));
tPAddr	gaiStaticAllocPages[NUM_STATIC_ALLOC] = {
	(tPAddr)(&gStaticAllocPages[0] - KERNEL_BASE),
	(tPAddr)(&gStaticAllocPages[1] - KERNEL_BASE),
	(tPAddr)(&gStaticAllocPages[2] - KERNEL_BASE),
	(tPAddr)(&gStaticAllocPages[3] - KERNEL_BASE)
};
extern char	gKernelEnd[];

#include <tpl_mm_phys_bitmap.h>

void MM_SetupPhys(void)
{
	MM_Tpl_InitPhys( 16*1024*1024/0x1000, NULL );
}

int MM_int_GetMapEntry( void *Data, int Index, tPAddr *Start, tPAddr *Length )
{
	switch(Index)
	{
	case 0:
		*Start = ((tVAddr)&gKernelEnd - KERNEL_BASE + 0xFFF) & ~0xFFF;
		*Length = 16*1024*1024;
		return 1;
	default:
		return 0;
	}
}

/**
 * \brief Takes a physical address and returns the ID of its range
 * \param Addr	Physical address of page
 * \return Range ID from eMMPhys_Ranges
 */
int MM_int_GetRangeID( tPAddr Addr )
{
	return MM_RANGE_MAX;	// ARM doesn't need ranges
}
