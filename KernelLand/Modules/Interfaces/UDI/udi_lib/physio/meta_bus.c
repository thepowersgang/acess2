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

#define USE_MEI	0

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
#if !USE_MEI
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
	LOG("cb=%p{...}", cb);
	PREP_OPS(udi_bus_bridge_ops_t, &cMetaLang_BusBridge, UDI_BUS_BRIDGE_OPS_NUM)
	
	UDI_int_MakeDeferredCb( UDI_GCB(cb), (udi_op_t*)ops->bus_bind_req_op );
}

struct marshalled_bus_bind_ack
{
	tUDI_DeferredCall	Call;
	udi_dma_constraints_t	dma_constraints;
	udi_ubit8_t	preferred_endianness;
	udi_status_t	status;
};

static void _unmarshal_bus_bind_ack(tUDI_DeferredCall *Call)
{
	LOG("Call=%p", Call);
	struct marshalled_bus_bind_ack *info = (void*)Call;
	UDI_int_ChannelReleaseFromCall( Call->cb );
	((udi_bus_bind_ack_op_t*)Call->Handler)(
		UDI_MCB(Call->cb, udi_bus_bind_cb_t),
		info->dma_constraints,
		info->preferred_endianness,
		info->status);
	free(info);
}

void udi_bus_bind_ack(
	udi_bus_bind_cb_t	*cb,
	udi_dma_constraints_t	dma_constraints,
	udi_ubit8_t	preferred_endianness,
	udi_status_t	status
	)
{
	LOG("cb=%p{...}, dma_constraints=%p, preferred_endianness=%i,status=%i",
		cb, dma_constraints, preferred_endianness, status);
	PREP_OPS(udi_bus_device_ops_t, &cMetaLang_BusBridge, UDI_BUS_DEVICE_OPS_NUM)
	
	struct marshalled_bus_bind_ack *call = NEW(struct marshalled_bus_bind_ack,);
	call->Call.Unmarshal = _unmarshal_bus_bind_ack;
	call->Call.cb = UDI_GCB(cb);
	call->Call.Handler = (udi_op_t*)ops->bus_bind_ack_op;
	call->dma_constraints = dma_constraints;
	call->preferred_endianness = preferred_endianness;
	call->status = status;
	UDI_int_AddDeferred(&call->Call);
}

void udi_intr_attach_req(udi_intr_attach_cb_t *cb)
{
	LOG("cb=%p", cb);
	PREP_OPS(udi_bus_bridge_ops_t, &cMetaLang_BusBridge, UDI_BUS_BRIDGE_OPS_NUM)
	UDI_int_MakeDeferredCb( UDI_GCB(cb), (udi_op_t*)ops->intr_attach_req_op );
}
void udi_intr_attach_ack(udi_intr_attach_cb_t *cb, udi_status_t status)
{
	LOG("cb=%p,status=%i", cb, status);
	PREP_OPS(udi_bus_device_ops_t, &cMetaLang_BusBridge, UDI_BUS_DEVICE_OPS_NUM)
	UDI_int_MakeDeferredCbS( UDI_GCB(cb), (udi_op_t*)ops->intr_attach_ack_op, status );
}

void udi_intr_detach_req(udi_intr_detach_cb_t *cb)
{
	LOG("cb=%p", cb);
	PREP_OPS(udi_bus_bridge_ops_t, &cMetaLang_BusBridge, UDI_BUS_BRIDGE_OPS_NUM)
	UDI_int_MakeDeferredCb( UDI_GCB(cb), (udi_op_t*)ops->intr_detach_req_op );
}
void udi_intr_detach_ack(udi_intr_detach_cb_t *cb)
{
	LOG("cb=%p", cb);
	PREP_OPS(udi_bus_device_ops_t, &cMetaLang_BusBridge, UDI_BUS_DEVICE_OPS_NUM)
	UDI_int_MakeDeferredCb( UDI_GCB(cb), (udi_op_t*)ops->intr_detach_ack_op );
}
#endif

void udi_intr_event_ind(udi_intr_event_cb_t *cb, udi_ubit8_t flags)
{
	LOG("cb=%p,flags=0x%x", cb, flags);
	PREP_OPS(udi_intr_handler_ops_t, &cMetaLang_BusBridge, UDI_BUS_INTR_HANDLER_OPS_NUM)
	UDI_int_MakeDeferredCbU8( UDI_GCB(cb), (udi_op_t*)ops->intr_event_ind_op, flags );
}

void udi_intr_event_rdy(udi_intr_event_cb_t *cb)
{
	LOG("cb=%p", cb);
	PREP_OPS(udi_intr_dispatcher_ops_t, &cMetaLang_BusBridge, UDI_BUS_INTR_DISPATCH_OPS_NUM)
	UDI_int_MakeDeferredCb( UDI_GCB(cb), (udi_op_t*)ops->intr_event_rdy_op );
}

void udi_intr_attach_ack_unused(udi_intr_attach_cb_t *intr_attach_cb, udi_status_t status)
{
	Log_Error("UDI", "Driver %s caused %s to be called",
		UDI_int_ChannelGetInstance(UDI_GCB(intr_attach_cb), false, NULL)->Module->ModuleName,
		__func__);
}
void udi_intr_detach_ack_unused(udi_intr_detach_cb_t *intr_detach_cb)
{
	Log_Error("UDI", "Driver %s caused %s to be called",
		UDI_int_ChannelGetInstance(UDI_GCB(intr_detach_cb), false, NULL)->Module->ModuleName,
		__func__);
}

#if USE_MEI
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
UDI_MEI_STUBS(udi_intr_detach_ack, udi_intr_detach_cb_t, 0,
	(), (), (),
	UDI_BUS_DEVICE_OPS_NUM, 4)

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

#endif

// === GLOBALS ==
udi_layout_t	udi_meta_info__bridge__bus_bind_cb[] = {
	UDI_DL_END
};
udi_layout_t	udi_meta_info__bridge__intr_attach_cb[] = {
	UDI_DL_STATUS_T,
	UDI_DL_UBIT8_T,
	UDI_DL_PIO_HANDLE_T,
	UDI_DL_END
};
udi_layout_t	udi_meta_info__bridge__intr_detach_cb[] = {
	UDI_DL_INDEX_T,
	UDI_DL_END
};
udi_layout_t	udi_meta_info__bridge__intr_event_cb[] = {
	UDI_DL_END
};

udi_layout_t	_BUS_BIND_cb_layout[] = {
	UDI_DL_END
};

#if USE_MEI
udi_layout_t	_noargs_marshal[] = {
	UDI_DL_END
};
udi_layout_t	_bus_bind_ack_marshal[] = {
	UDI_DL_DMA_CONSTRAINTS_T,
	UDI_DL_UBIT8_T,
	UDI_DL_STATUS_T,
	UDI_DL_END
};
udi_layout_t	_udi_intr_attach_ack_marshal[] = {
	UDI_DL_STATUS_T,
	UDI_DL_END
};

#define UDI__OPS_NUM	0
#define MEI_OPINFO(name,cat,flags,cbtype,rsp_ops,rsp_idx,err_ops,err_idx)	\
	{#name, UDI_MEI_OPCAT_##cat,flags,UDI_##cbtype##_CB_NUM, \
		UDI_##rsp_ops##_OPS_NUM,rsp_idx,UDI_##err_ops##_OPS_NUM,err_idx, \
		name##_direct, name##_backend, _##cbtype##_cb_layout, _##name##_marshal_layout }

udi_mei_op_template_t	udi_meta_info__bridge__bus_ops[] = {
	#define _udi_bus_bind_req_marshal_layout	_noargs_marshal
	MEI_OPINFO(udi_bus_bind_req, REQ, 0, BUS_BIND, BUS_DEVICE,1, ,0),
//	{"udi_bus_bind_req", UDI_MEI_OPCAT_REQ, 0, UDI_BUS_BIND_CB_NUM, UDI_BUS_DEVICE_OPS_NUM,1, 0,0,
//		udi_bus_bind_req_direct, udi_bus_bind_req_backend, udi_meta_info__bridge__bus_bind_cb,
//		_noargs_marshal},
	{"udi_bus_unbind_req", UDI_MEI_OPCAT_REQ, 0, UDI_BUS_BIND_CB_NUM, UDI_BUS_DEVICE_OPS_NUM,2, 0,0,
		udi_bus_unbind_req_direct, udi_bus_unbind_req_backend, udi_meta_info__bridge__bus_bind_cb,
		_noargs_marshal},
	{"udi_intr_attach_req", UDI_MEI_OPCAT_REQ, 0, UDI_BUS_INTR_ATTACH_CB_NUM, UDI_BUS_DEVICE_OPS_NUM,3, 0,0,
		udi_intr_attach_req_direct, udi_intr_attach_req_backend, udi_meta_info__bridge__intr_attach_cb,
		_noargs_marshal},
	{"udi_intr_detach_req", UDI_MEI_OPCAT_REQ, 0, UDI_BUS_INTR_DETACH_CB_NUM, UDI_BUS_DEVICE_OPS_NUM,4, 0,0,
		udi_intr_detach_req_direct, udi_intr_detach_req_backend, udi_meta_info__bridge__intr_detach_cb,
		_noargs_marshal},
	{0}
};
udi_mei_op_template_t	udi_meta_info__bridge__device_ops[] = {
	{"udi_bus_bind_ack", UDI_MEI_OPCAT_ACK, 0, UDI_BUS_BIND_CB_NUM, 0,0, 0,0,
		udi_bus_bind_ack_direct, udi_bus_bind_ack_backend, udi_meta_info__bridge__bus_bind_cb,
		_bus_bind_ack_marshal},
	{"udi_bus_unbind_ack", UDI_MEI_OPCAT_ACK, 0, UDI_BUS_BIND_CB_NUM, 0,0, 0,0,
		udi_bus_unbind_ack_direct, udi_bus_unbind_ack_backend, udi_meta_info__bridge__bus_bind_cb,
		_noargs_marshal},
	{"udi_intr_attach_ack", UDI_MEI_OPCAT_ACK, 0, UDI_BUS_INTR_ATTACH_CB_NUM, 0,0, 0,0,
		udi_intr_attach_ack_direct, udi_intr_attach_ack_backend, udi_meta_info__bridge__intr_attach_cb,
		_udi_intr_attach_ack_marshal},
	{"udi_intr_detach_ack", UDI_MEI_OPCAT_ACK, 0, UDI_BUS_INTR_DETACH_CB_NUM, 0,0, 0,0,
		udi_intr_detach_ack_direct, udi_intr_detach_ack_backend, udi_meta_info__bridge__intr_detach_cb,
		_noargs_marshal},
	{0}
};
udi_mei_ops_vec_template_t	udi_meta_info__bridge_ops[] = {
	{UDI_BUS_BRIDGE_OPS_NUM, UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_BIND, udi_meta_info__bridge__bus_ops},
	{UDI_BUS_DEVICE_OPS_NUM, UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_BIND, udi_meta_info__bridge__device_ops},
	{0}
};
udi_mei_init_t	udi_meta_info__bridge = {
	udi_meta_info__bridge_ops,
	NULL
};
#endif
tUDI_MetaLang	cMetaLang_BusBridge = {
	"udi_bridge",
	#if USE_MEI
	&udi_meta_info__bridge,
	#else
	NULL,
	#endif
	// CB Types
	5,
	{
		{0},	// 0: Empty
		{sizeof(udi_bus_bind_cb_t),    0, udi_meta_info__bridge__bus_bind_cb},
		{sizeof(udi_intr_attach_cb_t), 0, udi_meta_info__bridge__intr_attach_cb},
		{sizeof(udi_intr_detach_cb_t), 0, udi_meta_info__bridge__intr_detach_cb},
		{sizeof(udi_intr_event_cb_t),  0, NULL}
	}
};
