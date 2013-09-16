/*
 */
#include <acess.h>
#include <udi.h>
#include <udi_scsi.h>

// === EXPORTS ===
EXPORT(udi_scsi_bind_req);
EXPORT(udi_scsi_bind_ack);
EXPORT(udi_scsi_unbind_req);
EXPORT(udi_scsi_unbind_ack);
EXPORT(udi_scsi_io_req);
EXPORT(udi_scsi_io_ack);
EXPORT(udi_scsi_io_nak);
EXPORT(udi_scsi_ctl_req);
EXPORT(udi_scsi_ctl_ack);
EXPORT(udi_scsi_event_ind);
EXPORT(udi_scsi_event_ind_unused);
EXPORT(udi_scsi_event_res);
EXPORT(udi_scsi_inquiry_to_string);

// === CODE ===
void udi_scsi_bind_req(udi_scsi_bind_cb_t *cb,
	udi_ubit16_t bind_flags, udi_ubit16_t queue_depth,
	udi_ubit16_t max_sense_len, udi_ubit16_t aen_buf_size)
{
	UNIMPLEMENTED();
}
void udi_scsi_bind_ack(udi_scsi_bind_cb_t *cb, udi_ubit32_t hd_timeout_increase, udi_status_t status)
{
	UNIMPLEMENTED();
}
void udi_scsi_unbind_req(udi_scsi_bind_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_scsi_unbind_ack(udi_scsi_bind_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_scsi_io_req(udi_scsi_io_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_scsi_io_ack(udi_scsi_io_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_scsi_io_nak(udi_scsi_io_cb_t *cb, udi_scsi_status_t status, udi_buf_t *sense_buf)
{
	UNIMPLEMENTED();
}
void udi_scsi_ctl_req(udi_scsi_ctl_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_scsi_ctl_ack(udi_scsi_ctl_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}
void udi_scsi_event_ind(udi_scsi_event_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_scsi_event_ind_unused(udi_scsi_event_cb_t *cb)
{
	UNIMPLEMENTED();
	udi_scsi_event_res(cb);
}
void udi_scsi_event_res(udi_scsi_event_cb_t *cb)
{
	UNIMPLEMENTED();
}
void udi_scsi_inquiry_to_string(const udi_ubit8_t *inquiry_data, udi_size_t inquiry_len, char *str)
{
	UNIMPLEMENTED();
}
