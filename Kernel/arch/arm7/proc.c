/*
 * Acess2
 * - By John Hodge (thePowersGang)
 *
 * arch/arm7/proc.
 * - ARM7 Process Switching
 */
#include <acess.h>
#include <threads_int.h>

// === PROTOTYPES ===

// === GLOBALS ===
tThread	*gpCurrentThread;

// === CODE ===
void Proc_Start(void)
{
}

tThread *Proc_GetCurThread(void)
{
	return gpCurrentThread;
}
