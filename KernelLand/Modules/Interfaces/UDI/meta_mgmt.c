/**
 * \file meta_mgmt.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>

// === EXPORTS ===
EXPORT(udi_devmgmt_req);
EXPORT(udi_devmgmt_ack);
EXPORT(udi_final_cleanup_req);
EXPORT(udi_final_cleanup_ack);
EXPORT(udi_static_usage);
EXPORT(udi_enumerate_no_children);

// === CODE ===
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

void udi_static_usage(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	UNIMPLEMENTED();
}

void udi_enumerate_no_children(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level)
{
	UNIMPLEMENTED();
}
