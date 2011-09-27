/*
 * Acess2
 *
 * ARM7 Time code
 * arch/arm7/time.c
 */
#include <acess.h>

// === GLOBALS ===
tTime	giTimestamp;

// === CODE ===
tTime now(void)
{
	return giTimestamp;
}
