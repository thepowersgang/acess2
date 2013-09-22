/*
 * Acess2
 *
 * ARM7 Time code
 * arch/arm7/time.c
 */
#include <acess.h>

// === IMPORTS ===
extern tTime	Time_GetTickOffset(void);

// === GLOBALS ===
tTime	giTimestamp;

// === CODE ===
tTime now(void)
{
	return giTimestamp + Time_GetTickOffset();
}
