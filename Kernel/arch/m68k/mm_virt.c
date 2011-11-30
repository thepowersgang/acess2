/*
 * Acess2 M68K port
 * - By John Hodge (thePowersGang)
 *
 * arch/m68k/mm_virt.c
 * - Stubbed virtual memory management (no MMU)
 */
#include <acess.h>
#include <mm_virt.h>
#include <hal_proc.h>

// === CODE ===
tPAddr MM_GetPhysAddr(tVAddr Addr)
{
	return Addr;
}

void MM_SetFlags(tVAddr Addr, Uint Val, Uint Mask)
{
	return ;
}

Uint MM_GetFlags(tVAddr Addr)
{
	return 0;
}

int MM_Map(tVAddr Dest, tPAddr Src)
{
	Dest &= (PAGE_SIZE-1);
	Src &= (PAGE_SIZE-1);
	if(Dest != Src)
		memcpy((void*)Dest, (void*)Src, PAGE_SIZE);
	return 0;
}

tPAddr MM_Allocate(tVAddr Dest)
{
	return Dest;
}

void MM_Deallocate(tVAddr Addr)
{
}

void MM_ClearUser(void)
{
}

void MM_DumpTables(tVAddr Start, tVAddr End)
{

}

