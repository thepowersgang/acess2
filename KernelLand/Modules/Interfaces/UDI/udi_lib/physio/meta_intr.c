/**
 * \file physio/meta_intr.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_physio.h>
#include "../../udi_internal.h"

// === EXPORTS ===
EXPORT(udi_intr_attach_req);
EXPORT(udi_intr_attach_ack);
EXPORT(udi_intr_attach_ack_unused);
EXPORT(udi_intr_detach_req);
EXPORT(udi_intr_detach_ack);
EXPORT(udi_intr_detach_ack_unused);
EXPORT(udi_intr_event_ind);
EXPORT(udi_intr_event_rdy);

// === CODE ===
void udi_intr_attach_req(udi_intr_attach_cb_t *intr_attach_cb)
{
	UNIMPLEMENTED();
}
void udi_intr_attach_ack(udi_intr_attach_cb_t *intr_attach_cb, udi_status_t status)
{
	UNIMPLEMENTED();
}
void udi_intr_attach_ack_unused(udi_intr_attach_cb_t *intr_attach_cb, udi_status_t status)
{
	Log_Error("UDI", "Driver %s caused %s to be called",
		UDI_int_ChannelGetInstance(UDI_GCB(intr_attach_cb), false, NULL)->Module->ModuleName,
		__func__);
}

void udi_intr_detach_req(udi_intr_detach_cb_t *intr_detach_cb)
{
	UNIMPLEMENTED();
}
void udi_intr_detach_ack(udi_intr_detach_cb_t *intr_detach_cb)
{
	UNIMPLEMENTED();
}
void udi_intr_detach_ack_unused(udi_intr_detach_cb_t *intr_detach_cb)
{
	Log_Error("UDI", "Driver %s caused %s to be called",
		UDI_int_ChannelGetInstance(UDI_GCB(intr_detach_cb), false, NULL)->Module->ModuleName,
		__func__);
}

void udi_intr_event_ind(udi_intr_event_cb_t *intr_event_cb, udi_ubit8_t flags)
{
	UNIMPLEMENTED();
}

void udi_intr_event_rdy(udi_intr_event_cb_t *intr_event_cb)
{
	UNIMPLEMENTED();
}
