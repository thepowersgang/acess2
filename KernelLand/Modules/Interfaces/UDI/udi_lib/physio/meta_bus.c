/**
 * \file physio/meta_bus.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_physio.h>

// === EXPORTS ===
EXPORT(udi_bus_unbind_req);
EXPORT(udi_bus_unbind_ack);
EXPORT(udi_bus_bind_req);
EXPORT(udi_bus_bind_ack);

// === CODE ===
void udi_bus_unbind_req(udi_bus_bind_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_bus_unbind_ack(udi_bus_bind_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_bus_bind_req(udi_bus_bind_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_bus_bind_ack(
	udi_bus_bind_cb_t	*cb,
	udi_dma_constraints_t	dma_constraints,
	udi_ubit8_t	preferred_endianness,
	udi_status_t	status
	)
{
	UNIMPLEMENTED();
}
