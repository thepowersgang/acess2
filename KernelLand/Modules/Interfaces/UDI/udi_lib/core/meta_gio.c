/**
 * \file meta_gio.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>	// EXPORT
#include <udi.h>
#include <udi_internal.h>	// tUDI_MetaLang

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

extern udi_mei_init_t	udi_mei_info__gio;
#define udi_mei_info	udi_mei_info__gio

// === CODE ===
UDI_MEI_STUBS(udi_gio_bind_req, udi_gio_bind_cb_t,
	0, (), (), (),
	UDI_GIO_PROVIDER_OPS_NUM, 1)
udi_layout_t	_udi_gio_bind_req_marshal_layout[] = { UDI_DL_END };

UDI_MEI_STUBS(udi_gio_bind_ack, udi_gio_bind_cb_t,
	3, (device_size_lo, device_size_hi, status), (udi_ubit32_t, udi_ubit32_t, udi_status_t), (UDI_VA_UBIT32_T, UDI_VA_UBIT32_T, UDI_VA_STATUS_T),
	UDI_GIO_CLIENT_OPS_NUM, 1)
udi_layout_t	_udi_gio_bind_ack_marshal_layout[] = { UDI_DL_UBIT32_T, UDI_DL_UBIT32_T, UDI_DL_INDEX_T, UDI_DL_END };



UDI_MEI_STUBS(udi_gio_unbind_req, udi_gio_bind_cb_t,
	0, (), (), (),
	UDI_GIO_PROVIDER_OPS_NUM, 2)
udi_layout_t	_udi_gio_unbind_req_marshal_layout[] = { UDI_DL_END };

UDI_MEI_STUBS(udi_gio_unbind_ack, udi_gio_bind_cb_t,
	0, (), (), (),
	UDI_GIO_CLIENT_OPS_NUM, 2)
udi_layout_t	_udi_gio_unbind_ack_marshal_layout[] = { UDI_DL_END };


UDI_MEI_STUBS(udi_gio_xfer_req, udi_gio_xfer_cb_t,
	0, (), (), (),
	UDI_GIO_PROVIDER_OPS_NUM, 3)
udi_layout_t	_udi_gio_xfer_req_marshal_layout[] = { UDI_DL_END };

UDI_MEI_STUBS(udi_gio_xfer_ack, udi_gio_xfer_cb_t,
	0, (), (), (),
	UDI_GIO_CLIENT_OPS_NUM, 3)
udi_layout_t	_udi_gio_xfer_ack_marshal_layout[] = { UDI_DL_END };

UDI_MEI_STUBS(udi_gio_xfer_nak, udi_gio_xfer_cb_t,
	1, (status), (udi_status_t), (UDI_VA_STATUS_T),
	UDI_GIO_CLIENT_OPS_NUM, 4)
udi_layout_t	_udi_gio_xfer_nak_marshal_layout[] = { UDI_DL_STATUS_T, UDI_DL_END };


UDI_MEI_STUBS(udi_gio_event_res, udi_gio_event_cb_t,
	0, (), (), (),
	UDI_GIO_PROVIDER_OPS_NUM, 4)
udi_layout_t	_udi_gio_event_res_marshal_layout[] = { UDI_DL_END };

void udi_gio_event_ind_unused(udi_gio_event_cb_t *cb)
{
	UNIMPLEMENTED();
}

UDI_MEI_STUBS(udi_gio_event_ind, udi_gio_event_cb_t,
	0, (), (), (),
	UDI_GIO_CLIENT_OPS_NUM, 5)
udi_layout_t	_udi_gio_event_ind_marshal_layout[] = { UDI_DL_END };

void udi_gio_event_res_unused(udi_gio_event_cb_t *cb)
{
	UNIMPLEMENTED();
}

// -----------
// 1: UDI_GIO_BIND_CB_NUM
udi_layout_t	_GIO_BIND_cb_layout[] = {
	//UDI_DL_XFER_CONSTRAINTS,
		UDI_DL_UBIT32_T,
		UDI_DL_UBIT32_T,
		UDI_DL_UBIT32_T,
		UDI_DL_BOOLEAN_T,
		UDI_DL_BOOLEAN_T,
		UDI_DL_BOOLEAN_T,
	UDI_DL_END
};
// 2: UDI_GIO_XFER_CB_NUM
udi_layout_t	_GIO_XFER_cb_layout[] = {
	UDI_DL_UBIT8_T,	// udi_gio_op_t
	UDI_DL_INLINE_DRIVER_TYPED,
	UDI_DL_BUF,
		offsetof(udi_gio_xfer_cb_t, op),
		UDI_GIO_DIR_WRITE,
		UDI_GIO_DIR_WRITE,
	UDI_DL_END
};
// 3: UDI_GIO_EVENT_CB_NUM
udi_layout_t	_GIO_EVENT_cb_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_INLINE_DRIVER_TYPED,
	UDI_DL_END
};

// -----------
#define UDI__OPS_NUM	0
#define MEI_OPINFO(name,cat,flags,cbtype,rsp_ops,rsp_idx,err_ops,err_idx)	\
	{#name, UDI_MEI_OPCAT_##cat,flags,UDI_##cbtype##_CB_NUM, \
		UDI_##rsp_ops##_OPS_NUM,rsp_idx,UDI_##err_ops##_OPS_NUM,err_idx, \
		name##_direct, name##_backend, _##cbtype##_cb_layout, _##name##_marshal_layout }
udi_mei_op_template_t	udi_mei_info__gio__prov_ops[] = {
	MEI_OPINFO(udi_gio_bind_req, REQ, 0, GIO_BIND, GIO_CLIENT,1, ,0),
	MEI_OPINFO(udi_gio_unbind_req, REQ, 0, GIO_BIND, GIO_CLIENT,2, ,0),
	MEI_OPINFO(udi_gio_xfer_req, REQ, UDI_MEI_OP_ABORTABLE, GIO_XFER, GIO_CLIENT,3, GIO_CLIENT,4),
	MEI_OPINFO(udi_gio_event_res, RES, 0, GIO_EVENT, ,0, ,0),
	{0}
};
udi_mei_op_template_t	udi_mei_info__gio__client_ops[] = {
	MEI_OPINFO(udi_gio_bind_ack, ACK, 0, GIO_BIND, ,0, ,0),
	MEI_OPINFO(udi_gio_unbind_ack, ACK, 0, GIO_BIND, ,0, ,0),
	MEI_OPINFO(udi_gio_xfer_ack, ACK, 0, GIO_XFER, ,0, ,0),
	MEI_OPINFO(udi_gio_xfer_nak, ACK, 0, GIO_XFER, ,0, ,0),
	MEI_OPINFO(udi_gio_event_ind, IND, 0, GIO_EVENT, GIO_PROVIDER,4, ,0),
	{0}
};
udi_mei_ops_vec_template_t	udi_mei_info__gio_ops[] = {
	{UDI_GIO_PROVIDER_OPS_NUM, UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_BIND|UDI_MEI_REL_INITIATOR, udi_mei_info__gio__prov_ops},
	{UDI_GIO_CLIENT_OPS_NUM, UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_BIND, udi_mei_info__gio__client_ops},
	{0}
};
udi_mei_init_t	udi_mei_info__gio = {
	udi_mei_info__gio_ops,
	NULL
};
tUDI_MetaLang	cMetaLang_GIO = {
	"udi_gio",
	&udi_mei_info__gio,
	4,
	{
		{0},
		{sizeof(udi_gio_bind_cb_t),  0, _GIO_BIND_cb_layout},
		{sizeof(udi_gio_xfer_cb_t),  0, _GIO_XFER_cb_layout},
		{sizeof(udi_gio_event_cb_t), 0, _GIO_EVENT_cb_layout},
	}
};
