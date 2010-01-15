/**
 * \file mem.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_mem.h>

// === CODE ===
void udi_mem_alloc(
	udi_mem_alloc_call_t *callback,
	udi_cb_t	*gcb,
	udi_size_t	size,
	udi_ubit8_t	flags
	)
{
	void	*buf = malloc(size);
	if(buf)
	{
		if( !(flags & UDI_MEM_NOZERO) )
			memset(buf, 0, size);
	}
	callback(gcb, buf);
}

void udi_mem_free(void *target_mem)
{
	free(target_mem);
}

// === EXPORTS ===
EXPORT(udi_mem_alloc);
EXPORT(udi_mem_free);
