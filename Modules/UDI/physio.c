/**
 * \file physio.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_physio.h>

// === EXPORTS ===
EXPORT(udi_dma_constraints_attr_reset);
EXPORT(udi_dma_constraints_free);

// === CODE ===
void udi_dma_constraints_attr_reset(
	udi_dma_constraints_t	constraints,
	udi_dma_constraints_attr_t	attr_type
	)
{
	UNIMPLEMENTED();
}

void udi_dma_constraints_free(udi_dma_constraints_t constraints)
{
	UNIMPLEMENTED();
}
