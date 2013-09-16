/**
 * \file physio.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_physio.h>

// === EXPORTS ===
EXPORT(udi_dma_constraints_attr_set);
EXPORT(udi_dma_constraints_attr_reset);
EXPORT(udi_dma_constraints_free);

// === CODE ===
void udi_dma_constraints_attr_set(udi_dma_constraints_attr_set_call_t *callback, udi_cb_t *gcb,
	udi_dma_constraints_t src_constraints,
	const udi_dma_constraints_attr_spec_t *attr_list, udi_ubit16_t list_length,
	udi_ubit8_t flags)
{
	UNIMPLEMENTED();
}

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
