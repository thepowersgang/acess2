/**
 * \file meta_gio.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_meta_gio.h>

// === EXPORTS ===
EXPORT(udi_gio_bind_req);
EXPORT(udi_gio_bind_ack);
EXPORT(udi_gio_unbind_req);
EXPORT(udi_gio_unbind_ack);
EXPORT(udi_gio_xfer_req);
EXPORT(udi_gio_xfer_ack);
EXPORT(udi_gio_xfer_nak);
EXPORT(udi_gio_event_res);
EXPORT(udi_gio_event_ind);
EXPORT(udi_gio_event_res_unused);

// === CODE ===
void udi_gio_bind_req(udi_gio_bind_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_gio_bind_ack(
	udi_gio_bind_cb_t	*cb,
	udi_ubit32_t	device_size_lo,
	udi_ubit32_t	device_size_hi,
	udi_status_t	status
	)
{
	UNIMPLEMENTED();
}

void udi_gio_unbind_req(udi_gio_bind_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_gio_unbind_ack(udi_gio_bind_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_gio_xfer_req(udi_gio_xfer_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_gio_xfer_ack(udi_gio_xfer_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_gio_xfer_nak(udi_gio_xfer_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}

void udi_gio_event_res(udi_gio_event_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_gio_event_ind(udi_gio_event_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_gio_event_res_unused(udi_gio_event_cb_t *cb)
{
	UNIMPLEMENTED();
}
