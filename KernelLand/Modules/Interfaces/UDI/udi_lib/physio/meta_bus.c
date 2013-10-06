/**
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * udi_lib/physio/meta_bus.c
 * - Bus Bridge Metalanguage
 */
#define DEBUG	1
#include <acess.h>
#include <udi.h>
#include <udi_physio.h>
#include "../../udi_internal.h"

// === GLOBALS ==
udi_layout_t	cMetaLang_BusBridge_IntrAttachCbLayout[] = {
	UDI_DL_STATUS_T,
	UDI_DL_UBIT8_T,
	UDI_DL_ORIGIN_T,	// TODO: handle
	0
};
tUDI_MetaLang	cMetaLang_BusBridge = {
	"udi_bridge",
	// CB Types
	5,
	{
		{0},	// 0: Empty
		{sizeof(udi_bus_bind_cb_t), NULL},	// Defined, but is just a gcb
		{sizeof(udi_intr_attach_cb_t), NULL},
		{sizeof(udi_intr_detach_cb_t), NULL},
		{sizeof(udi_intr_event_cb_t), NULL}
	}
};

// === EXPORTS ===
EXPORT(udi_bus_unbind_req);
EXPORT(udi_bus_unbind_ack);
EXPORT(udi_bus_bind_req);
EXPORT(udi_bus_bind_ack);

#define PREP_OPS(type,ml,num)	const type *ops = UDI_int_ChannelPrepForCall(UDI_GCB(cb), ml, num); \
	if(!ops) { Log_Warning("UDI", "%s on wrong channel type", __func__); return ; }

#define PREP_OPS_DEVICE	const udi_bus_device_ops_t *ops = UDI_int_ChannelPrepForCall( UDI_GCB(cb), &cMetaLang_BusBridge, UDI_BUS_DEVICE_OPS_NUM ); \
	if(!ops) { Log_Warning("UDI", "%s on wrong channel type", __func__); return ; }

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
