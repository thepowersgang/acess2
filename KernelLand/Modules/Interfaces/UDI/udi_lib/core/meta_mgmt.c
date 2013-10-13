/**
 * \file meta_mgmt.c
 * \author John Hodge (thePowersGang)
 */
#define DEBUG	0
#include <udi.h>
#include <acess.h>
#include <udi_internal.h>
#include <udi_internal_ma.h>

// === EXPORTS ===
EXPORT(udi_usage_ind);
EXPORT(udi_static_usage);
EXPORT(udi_usage_res);
EXPORT(udi_enumerate_req);
EXPORT(udi_enumerate_no_children);
EXPORT(udi_enumerate_ack);
EXPORT(udi_devmgmt_req);
EXPORT(udi_devmgmt_ack);
EXPORT(udi_final_cleanup_req);
EXPORT(udi_final_cleanup_ack);

tUDI_MetaLang	cMetaLang_Management = {
	"udi_mgmt",
	NULL,
	3,
	{
		{sizeof(udi_enumerate_cb_t),     0, NULL},
		{sizeof(udi_usage_cb_t),         0, NULL},
		{sizeof(udi_channel_event_cb_t), 0, NULL},
	}
};

// === CODE ===
void udi_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	LOG("cb=%p{...}, resource_level=%i", cb, resource_level);
	const udi_mgmt_ops_t *ops = UDI_int_ChannelPrepForCall( UDI_GCB(cb), &cMetaLang_Management, 0 );
	if(!ops) {
		Log_Warning("UDI", "%s on wrong channel type", __func__);
		return ;
	}

	// Non-deferred because it's usually called with a stack allocated cb	
	UDI_int_ChannelReleaseFromCall( UDI_GCB(cb) );
	ops->usage_ind_op(cb, resource_level);
}

void udi_static_usage(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	cb->trace_mask = 0;
	udi_usage_res(cb);
}

void udi_usage_res(udi_usage_cb_t *cb)
{
	// TODO: Update trace mask from cb
	LOG("cb=%p{cb->trace_mask=%x}", cb, cb->trace_mask);
	UDI_MA_TransitionState(UDI_GCB(cb)->initiator_context, UDI_MASTATE_USAGEIND, UDI_MASTATE_SECBIND);
	udi_cb_free( UDI_GCB(cb) );
}

void udi_enumerate_req(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level)
{
	LOG("cb=%p{...}, enumeration_level=%i", cb, enumeration_level);
	const udi_mgmt_ops_t *ops = UDI_int_ChannelPrepForCall( UDI_GCB(cb), &cMetaLang_Management, 0 );
	if(!ops) {
		Log_Warning("UDI", "%s on wrong channel type", __func__);
		return ;
	}
	
	UDI_int_MakeDeferredCbU8( UDI_GCB(cb), (udi_op_t*)ops->enumerate_req_op, enumeration_level );
}

void udi_enumerate_no_children(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level)
{
	udi_enumerate_ack(cb, UDI_ENUMERATE_LEAF, 0);
}

void udi_enumerate_ack(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_result, udi_index_t ops_idx)
{
	UDI_int_ChannelFlip( UDI_GCB(cb) );
	LOG("cb=%p, enumeration_result=%i, ops_idx=%i", cb, enumeration_result, ops_idx);
	switch( enumeration_result )
	{
	case UDI_ENUMERATE_OK:
		#if DEBUG && 0
		for( int i = 0; i < cb->attr_valid_length; i ++ )
		{
			udi_instance_attr_list_t	*at = &cb->attr_list[i];
			switch(at->attr_type)
			{
			case UDI_ATTR_STRING:
				LOG("[%i] %s String '%.*s'", i, at->attr_name,
					at->attr_length, at->attr_value);
				break;
			case UDI_ATTR_UBIT32:
				LOG("[%i] %s UBit32 0x%08x", i, at->attr_name,
					UDI_ATTR32_GET(at->attr_value));
				break;
			default:
				LOG("[%i] %s %i", i, at->attr_name,
					at->attr_type);
				break;
			}
		}
		#endif
		// Returned a device
		UDI_MA_AddChild(cb, ops_idx);
		udi_enumerate_req(cb, UDI_ENUMERATE_NEXT);
		return ;
	case UDI_ENUMERATE_LEAF:
	case UDI_ENUMERATE_DONE:
		// All done. Chain terminates
		UDI_MA_TransitionState(UDI_GCB(cb)->initiator_context,
			UDI_MASTATE_ENUMCHILDREN, UDI_MASTATE_ACTIVE);
		udi_cb_free( UDI_GCB(cb) );
		return ;
	default:
		Log_Notice("UDI", "Unknown enumeration_result %i", enumeration_result);
		return ;
	}
}

void udi_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID )
{	
	ENTER("pcb imgmt_op iparent_ID", cb, mgmt_op, parent_ID);
	LEAVE('-');
}

void udi_devmgmt_ack(udi_mgmt_cb_t *cb, udi_ubit8_t flags, udi_status_t status)
{
	ENTER("pcb xflags istatus", cb, flags, status);
	LEAVE('-');
}

void udi_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	ENTER("pcb", cb);
	LEAVE('-');
}

void udi_final_cleanup_ack(udi_mgmt_cb_t *cb)
{
	ENTER("pcb", cb);
	LEAVE('-');
}

