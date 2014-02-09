/**
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * udi_lib/physio/meta_bus.c
 * - Bus Bridge Metalanguage
 */
#define DEBUG	1
#include <udi.h>
#include <udi_physio.h>
#include <acess.h>
#include <udi_internal.h>

#define udi_mei_info	udi_meta_info__bridge

extern tUDI_MetaLang	cMetaLang_BusBridge;
extern udi_mei_init_t	udi_meta_info__bridge;

// === EXPORTS ===
EXPORT(udi_bus_bind_req);
EXPORT(udi_bus_bind_ack);
EXPORT(udi_bus_unbind_req);
EXPORT(udi_bus_unbind_ack);
EXPORT(udi_intr_attach_req);
EXPORT(udi_intr_attach_ack);
EXPORT(udi_intr_attach_ack_unused);
EXPORT(udi_intr_detach_req);
EXPORT(udi_intr_detach_ack);
EXPORT(udi_intr_detach_ack_unused);
EXPORT(udi_intr_event_ind);
EXPORT(udi_intr_event_rdy);

#define PREP_OPS(type,ml,num)	const type *ops = UDI_int_ChannelPrepForCall(UDI_GCB(cb), ml, num); \
	if(!ops) { Log_Warning("UDI", "%s on wrong channel type", __func__); return ; }

#define PREP_OPS_DEVICE	const udi_bus_device_ops_t *ops = UDI_int_ChannelPrepForCall( UDI_GCB(cb), &cMetaLang_BusBridge, UDI_BUS_DEVICE_OPS_NUM ); \
	if(!ops) { Log_Warning("UDI", "%s on wrong channel type", __func__); return ; }

// === CODE ===
UDI_MEI_STUBS(udi_bus_bind_ack, udi_bus_bind_cb_t, 3,
	(dma_constraints,       preferred_endianness, status      ),
	(udi_dma_constraints_t, udi_ubit8_t,          udi_status_t),
	(UDI_VA_POINTER,        UDI_VA_UBIT8_T,       UDI_VA_STATUS_T),
	UDI_BUS_DEVICE_OPS_NUM, 1)
UDI_MEI_STUBS(udi_bus_unbind_ack, udi_bus_bind_cb_t, 0,
	(), (), (),
	UDI_BUS_DEVICE_OPS_NUM, 2)
UDI_MEI_STUBS(udi_intr_attach_ack, udi_intr_attach_cb_t, 1,
	(status), (udi_status_t), (UDI_VA_STATUS_T),
	UDI_BUS_DEVICE_OPS_NUM, 3)
void udi_intr_attach_ack_unused(udi_intr_attach_cb_t *intr_attach_cb, udi_status_t status)
{
	Log_Error("UDI", "Driver %s caused %s to be called",
		UDI_int_ChannelGetInstance(UDI_GCB(intr_attach_cb), false, NULL)->Module->ModuleName,
		__func__);
}
UDI_MEI_STUBS(udi_intr_detach_ack, udi_intr_detach_cb_t, 0,
	(), (), (),
	UDI_BUS_DEVICE_OPS_NUM, 4)
void udi_intr_detach_ack_unused(udi_intr_detach_cb_t *intr_detach_cb)
{
	Log_Error("UDI", "Driver %s caused %s to be called",
		UDI_int_ChannelGetInstance(UDI_GCB(intr_detach_cb), false, NULL)->Module->ModuleName,
		__func__);
}

UDI_MEI_STUBS(udi_bus_bind_req, udi_bus_bind_cb_t, 0,
	(), (), (),
	UDI_BUS_BRIDGE_OPS_NUM, 1)
UDI_MEI_STUBS(udi_bus_unbind_req, udi_bus_bind_cb_t, 0,
	(), (), (),
	UDI_BUS_BRIDGE_OPS_NUM, 2)
UDI_MEI_STUBS(udi_intr_attach_req, udi_intr_attach_cb_t, 0,
	(), (), (),
	UDI_BUS_BRIDGE_OPS_NUM, 3)
UDI_MEI_STUBS(udi_intr_detach_req, udi_intr_detach_cb_t, 0,
	(), (), (),
	UDI_BUS_BRIDGE_OPS_NUM, 4)

UDI_MEI_STUBS(udi_intr_event_rdy, udi_intr_event_cb_t, 0,
	(), (), (),
	UDI_BUS_INTR_DISPATCH_OPS_NUM, 1)
UDI_MEI_STUBS(udi_intr_event_ind, udi_intr_event_cb_t, 1,
	(flags), (udi_ubit8_t), (UDI_VA_UBIT8_T),
	UDI_BUS_INTR_HANDLER_OPS_NUM, 1)


// === GLOBALS ==
udi_layout_t	_BUS_BIND_cb_layout[] = {
	UDI_DL_END
};
udi_layout_t	_BUS_INTR_ATTACH_cb_layout[] = {
	UDI_DL_STATUS_T,
	UDI_DL_UBIT8_T,
	UDI_DL_PIO_HANDLE_T,
	UDI_DL_END
};
udi_layout_t	_BUS_INTR_DETACH_cb_layout[] = {
	UDI_DL_INDEX_T,
	UDI_DL_END
};
udi_layout_t	_BUS_INTR_EVENT_cb_layout[] = {
	UDI_DL_BUF,
		0,0,0,
	UDI_DL_UBIT16_T,
	UDI_DL_END
};

static const udi_layout_t	_noargs_marshal[] = {
	UDI_DL_END
};
#define _udi_bus_bind_req_marshal_layout	_noargs_marshal
#define _udi_bus_unbind_req_marshal_layout	_noargs_marshal
#define _udi_intr_attach_req_marshal_layout	_noargs_marshal
#define	_udi_intr_detach_req_marshal_layout	_noargs_marshal
const udi_layout_t	_udi_bus_bind_ack_marshal_layout[] = {
	UDI_DL_DMA_CONSTRAINTS_T,
	UDI_DL_UBIT8_T,
	UDI_DL_STATUS_T,
	UDI_DL_END
};
#define _udi_bus_unbind_ack_marshal_layout	_noargs_marshal
const udi_layout_t	_udi_intr_attach_ack_marshal_layout[] = {
	UDI_DL_STATUS_T,
	UDI_DL_END
};
#define	_udi_intr_detach_ack_marshal_layout	_noargs_marshal
#define _udi_intr_event_rdy_marshal_layout	_noargs_marshal
const udi_layout_t	_udi_intr_event_ind_marshal_layout[] = {
	UDI_DL_UBIT8_T,
	UDI_DL_END
};

#define UDI__OPS_NUM	0
#define MEI_OPINFO(name,cat,flags,cbtype,rsp_ops,rsp_idx,err_ops,err_idx)	\
	{#name, UDI_MEI_OPCAT_##cat,flags,UDI_##cbtype##_CB_NUM, \
		UDI_##rsp_ops##_OPS_NUM,rsp_idx,UDI_##err_ops##_OPS_NUM,err_idx, \
		name##_direct, name##_backend, _##cbtype##_cb_layout, _##name##_marshal_layout }

udi_mei_op_template_t	udi_meta_info__bridge__bus_ops[] = {
	MEI_OPINFO(udi_bus_bind_req, REQ, 0, BUS_BIND, BUS_DEVICE,1, ,0),
	MEI_OPINFO(udi_bus_unbind_req, REQ, 0, BUS_BIND, BUS_DEVICE,2, ,0),
	MEI_OPINFO(udi_intr_attach_req, REQ, 0, BUS_INTR_ATTACH, BUS_DEVICE,3, ,0),
	MEI_OPINFO(udi_intr_detach_req, REQ, 0, BUS_INTR_DETACH, BUS_DEVICE,4, ,0),
	{0}
};
udi_mei_op_template_t	udi_meta_info__bridge__device_ops[] = {
	MEI_OPINFO(udi_bus_bind_ack, ACK, 0, BUS_BIND, ,0, ,0),
	MEI_OPINFO(udi_bus_unbind_ack, ACK, 0, BUS_BIND, ,0, ,0),
	MEI_OPINFO(udi_intr_attach_ack, ACK, 0, BUS_INTR_ATTACH, ,0, ,0),
	MEI_OPINFO(udi_intr_detach_ack, ACK, 0, BUS_INTR_DETACH, ,0, ,0),
	{0}
};
udi_mei_op_template_t	udi_meta_info__bridge__intr_disp_ops[] = {
	MEI_OPINFO(udi_intr_event_rdy, RDY, 0, BUS_INTR_EVENT, ,0, ,0),
	{0}
};
udi_mei_op_template_t	udi_meta_info__bridge__intr_hdlr_ops[] = {
	MEI_OPINFO(udi_intr_event_ind, IND, 0, BUS_INTR_EVENT, ,0, ,0),
	{0}
};
udi_mei_ops_vec_template_t	udi_meta_info__bridge_ops[] = {
	{UDI_BUS_BRIDGE_OPS_NUM, UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_INITIATOR|UDI_MEI_REL_BIND, udi_meta_info__bridge__bus_ops},
	{UDI_BUS_DEVICE_OPS_NUM, UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_BIND, udi_meta_info__bridge__device_ops},
	{UDI_BUS_INTR_DISPATCH_OPS_NUM, UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_INITIATOR, udi_meta_info__bridge__intr_disp_ops},
	{UDI_BUS_INTR_HANDLER_OPS_NUM, UDI_MEI_REL_EXTERNAL, udi_meta_info__bridge__intr_hdlr_ops},
	{0}
};
udi_mei_init_t	udi_meta_info__bridge = {
	udi_meta_info__bridge_ops,
	NULL
};
tUDI_MetaLang	cMetaLang_BusBridge = {
	"udi_bridge",
	&udi_meta_info__bridge,
	// CB Types
	5,
	{
		{0},	// 0: Empty
		{sizeof(udi_bus_bind_cb_t),    0, _BUS_BIND_cb_layout},
		{sizeof(udi_intr_attach_cb_t), 0, _BUS_INTR_ATTACH_cb_layout},
		{sizeof(udi_intr_detach_cb_t), 0, _BUS_INTR_DETACH_cb_layout},
		{sizeof(udi_intr_event_cb_t),  0, _BUS_INTR_EVENT_cb_layout}
	}
};
