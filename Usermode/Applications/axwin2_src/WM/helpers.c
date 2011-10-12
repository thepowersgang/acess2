/*
 * Acess GUI (AxWin) Version 2
 * By John Hodge (thePowersGang)
 */
#include "common.h"

// === CODE ===
void memset32(void *ptr, uint32_t val, size_t count)
{
	uint32_t *dst = ptr;
	while(count --)	*dst++ = val;
}
