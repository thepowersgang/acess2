/*
 * Acess2 M68K port
 * - By John Hodge (thePowersGang)
 *
 * arch/m68k/mm_phys.c
 * - Stubbed physical memory management
 */
#include <acess.h>

// === CODE ===
void MM_RefPhys(tPAddr Page)
{
	// TODO: Refcount pages
	Log_Warning("MMPhys", "TODO: Implement MM_RefPhys");
}

int MM_SetPageNode(tPAddr Page, void *Node)
{
	Log_Warning("MMPhys", "TODO: Implement MM_SetPageNode");
	return -1;
}

