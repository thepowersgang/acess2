/**
 * \file meta_mgmt.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include "../udi_internal.h"

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

// === CODE ===
void udi_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	const udi_mgmt_ops_t *ops;
	if( !(ops = UDI_int_ChannelPrepForCall( UDI_GCB(cb), METALANG_MGMT, 0 )) ) {
		Log_Warning("UDI", "udi_usage_ind on wrong channel type");
		return ;
	}
	
	ops->usage_ind_op(cb, resource_level);
}

void udi_static_usage(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	UNIMPLEMENTED();
}

void udi_usage_res(udi_usage_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_enumerate_req(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level)
{
	UNIMPLEMENTED();
}

void udi_enumerate_no_children(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level)
{
	UNIMPLEMENTED();
}

void udi_enumerate_ack(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_result, udi_index_t ops_idx)
{
	UNIMPLEMENTED();
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

