/*
 * UDI Ne2000 NIC Driver
 * By John Hodge (thePowersGang)
 *
 * ne2000_core.c
 * - UDI initialisation 
 */
#include <udi.h>
#include <udi_nic.h>
#include "ne2000_common.h"

#define NE2K_META_BUS	1
#define NE2K_META_NIC	2

// === CODE ===
// --- Management
void ne2k_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
}
void ne2k_enumerate_req(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level)
{
	udi_instance_attr_list_t *attr_list = cb->attr_list;
	
	switch(enumeration_level)
	{
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:
		// TODO: Emit the ND binding
		DPT_SET_ATTR32(attr_list, "if_num", 0);
		attr_list ++;
		DPT_SET_ATTR_STRING(attr_list, "if_media", "eth");
		attr_list ++;
		//DPT_SET_ATTR_STRING(attr_list, "identifier", hexaddr);
		//attr_list ++;
		udi_enumerate_ack(cb, UDI_ENUMERATE_OK, 2);
		break
	case UDI_ENUMERATE_NEXT:
		udi_enumerate_ack(cb, UDI_ENUMERATE_DONE, 0);
		break;
	}
}
void ne2k_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID)
{
}
void ne2k_final_cleanup_req(udi_mgmt_cb_t *cb)
{
}
// --- Bus
void ne2k_bus_dev_channel_event_ind(udi_channel_event_cb_t *cb)
{
}
void ne2k_bus_dev_bus_bind_ack(udi_bus_bind_cb_t *cb,
	udi_dma_constraints_t dma_constraints, udi_ubit8_t perferred_endianness, udi_status_t status)
{
}
void ne2k_bus_dev_bus_unbind_ack(udi_bus_bind_cb_t *cb)
{
}
void ne2k_bus_dev_intr_attach_ack(udi_intr_attach_cb_t *intr_attach_cb, udi_status_t status)
{
}
void ne2k_bus_dev_intr_detach_ack(udi_intr_detach_cb_t *intr_detach_cb)
{
}
// --- ND Common
void ne2k_nd_ctrl_channel_event_ind(udi_channel_event_cb_t *cb)
{
}
void ne2k_nd_ctrl_bind_req(udi_nic_bind_cb_t *cb, udi_index_t tx_chan_index, udi_index_t rx_chan_index)
{
}
void ne2k_nd_ctrl_unbind_req(udi_nic_cb_t *cb)
{
}
void ne2k_nd_ctrl_enable_req(udi_nic_cb_t *cb)
{
}
void ne2k_nd_ctrl_disable_req(udi_nic_cb_t *cb)
{
}
void ne2k_nd_ctrl_ctrl_req(udi_nic_ctrl_cb_t *cb)
{
}
void ne2k_nd_ctrl_info_req(udi_nic_info_cb_t *cb, udi_boolean_t reset_statistics)
{
}

// === Definition structures ===
udi_mgmt_ops_t	ne2k_mgmt_ops = {
	ne2k_usage_ind,
	ne2k_enumerate_req,
	ne2k_devmgmt_req,
	ne2k_final_cleanup_req
};
udi_ubit8_t	ne2k_mgmt_op_flags[4] = {0,0,0,0};
udi_bus_device_ops_t	ne2k_bus_dev_ops = {
	ne2k_bus_dev_channel_event_ind,
	ne2k_bus_dev_bus_bind_ack,
	ne2k_bus_dev_bus_unbind_ack,
	ne2k_bus_dev_intr_attach_ack,
	ne2k_bus_dev_intr_detach_ack
};
udi_ubit8_t	ne2k_bus_dev_ops_flags[5] = {0};
udi_nd_ctrl_ops_t	ne2k_nd_ctrl_ops = {
	ne2k_nd_ctrl_channel_event_ind,
	ne2k_nd_ctrl_bind_req,
	ne2k_nd_ctrl_unbind_req,
	ne2k_nd_ctrl_enable_req,
	ne2k_nd_ctrl_disable_req,
	ne2k_nd_ctrl_ctrl_req,
	ne2k_nd_ctrl_info_req
};
udi_ubit8_t	ne2k_nd_ctrl_ops_flags[7] = {0};
udi_nd_tx_ops	ne2k_nd_tx_ops = {
	ne2k_nd_tx_channel_event_ind,
	ne2k_nd_tx_tx_req,
	ne2k_nd_tx_exp_tx_req
};
udi_ubit8_t	ne2k_nd_tx_ops_flags[3] = {0};
udi_nd_rx_ops	ne2k_nd_rx_ops = {
	ne2k_nd_rx_channel_event_ind,
	ne2k_nd_rx_rx_rdy
};
udi_ubit8_t	ne2k_nd_rx_ops_flags[2] = {0};
const udi_primary_init_t	ne2k_pri_init = {
	.mgmt_ops = &ne2k_mgmt_ops,
	.mgmt_op_flags = ne2k_mgmt_op_flags,
	.mgmt_scratch_requirement = 0,
	.enumeration_attr_list_length = 4,
	.rdata_size = sizeof(ne2k_rdata_t),
	.child_data_size = 0,
	.per_parent_paths = 0
};
const udi_ops_init_t	ne2k_ops_list[] = {
	{
		1, NE2K_META_BUS, UDI_BUS_DEVICE_OPS_NUM,
		0,
		(udi_ops_vector_t*)ne2k_bus_dev_ops,
		ne2k_bus_dev_ops_flags
	},
	{
		2, NE2K_META_NIC, UDI_ND_CTRL_OPS_NUM,
		0,
		(udi_ops_vector_t*)ne2k_nd_ctrl_ops,
		ne2k_nd_ctrl_ops_flags
	},
	{
		3, NE2K_META_NIC, UDI_ND_TX_OPS_NUM,
		0,
		(udi_ops_vector_t*)ne2k_nd_tx_ops,
		ne2k_nd_tx_ops_flags
	},
	{
		4, NE2K_META_NIC, UDI_ND_RX_OPS_NUM,
		0,
		(udi_ops_vector_t*)ne2k_nd_rx_ops,
		ne2k_nd_rx_ops_flags
	},
	{0}
}
const udi_init_t	udi_init_info = {
	.primary_init_info = &ne2k_pri_init,
	.ops_init_list = ne2k_ops_list
};
