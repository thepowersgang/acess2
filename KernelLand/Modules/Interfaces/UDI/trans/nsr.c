/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * trans/nsr.c
 * - "Network Service Requester" (UDI->IP Translation)
 */
#define DEBUG	1
#include <udi.h>
#include <udi_nic.h>
#include <acess.h>
#include <trans_nsr.h>
#include <IPStack/include/adapters_api.h>

enum {
	ACESSNSR_OPS_CTRL = 1,
	ACESSNSR_OPS_TX,
	ACESSNSR_OPS_RX,
};
enum {
	ACESSNSR_META_NIC = 1
};
enum {
	ACESSNSR_CB_CTRL = 1
};

// === TYPES ===
typedef struct
{
	udi_init_context_t	init_context;
} acessnsr_rdata_t;

// === PROTOTYPES ===
// --- Management metalang
void acessnsr_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level);
void acessnsr_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID);
void acessnsr_final_cleanup_req(udi_mgmt_cb_t *cb);
// --- NSR Control
void acessnsr_ctrl_channel_event_ind(udi_channel_event_cb_t *cb);
void acessnsr_ctrl_bind_ack(udi_nic_bind_cb_t *cb, udi_status_t status);
void acessnsr_ctrl_unbind_ack(udi_nic_cb_t *cb, udi_status_t status);
void acessnsr_ctrl_enable_ack(udi_nic_cb_t *cb, udi_status_t status);
void acessnsr_ctrl_disable_ack(udi_nic_cb_t *cb, udi_status_t status);
void acessnsr_ctrl_ctrl_ack(udi_nic_ctrl_cb_t *cb, udi_status_t status);
void acessnsr_ctrl_status_ind(udi_nic_status_cb_t *cb);
void acessnsr_ctrl_info_ack(udi_nic_info_cb_t *cb);
// --- NSR TX
void acessnsr_tx_channel_event_ind(udi_channel_event_cb_t *cb);
void acessnsr_tx_rdy(udi_nic_tx_cb_t *cb);
// --- NSR RX
void acessnsr_rx_channel_event_ind(udi_channel_event_cb_t *cb);
void acessnsr_rx_ind(udi_nic_rx_cb_t *cb);
void acessnsr_rx_exp_ind(udi_nic_rx_cb_t *cb);

// === CODE ===
// --- Management metalang
void acessnsr_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
}
void acessnsr_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID)
{
}
void acessnsr_final_cleanup_req(udi_mgmt_cb_t *cb)
{
}
// --- NSR Control
void acessnsr_ctrl_channel_event_ind(udi_channel_event_cb_t *cb)
{
	//acessnsr_rdata_t *rdata = UDI_GCB(cb)->context;
	switch(cb->event)
	{
	
	}
}
void acessnsr_ctrl_bind_ack(udi_nic_bind_cb_t *cb, udi_status_t status)
{
}
void acessnsr_ctrl_unbind_ack(udi_nic_cb_t *cb, udi_status_t status)
{
}
void acessnsr_ctrl_enable_ack(udi_nic_cb_t *cb, udi_status_t status)
{
}
void acessnsr_ctrl_disable_ack(udi_nic_cb_t *cb, udi_status_t status)
{
}
void acessnsr_ctrl_ctrl_ack(udi_nic_ctrl_cb_t *cb, udi_status_t status)
{
}
void acessnsr_ctrl_status_ind(udi_nic_status_cb_t *cb)
{
}
void acessnsr_ctrl_info_ack(udi_nic_info_cb_t *cb)
{
}
// --- NSR TX
void acessnsr_tx_channel_event_ind(udi_channel_event_cb_t *cb)
{
}
void acessnsr_tx_rdy(udi_nic_tx_cb_t *cb)
{
}
// --- NSR RX
void acessnsr_rx_channel_event_ind(udi_channel_event_cb_t *cb)
{
}
void acessnsr_rx_ind(udi_nic_rx_cb_t *cb)
{
}
void acessnsr_rx_exp_ind(udi_nic_rx_cb_t *cb)
{
}

// === UDI Bindings ===
udi_mgmt_ops_t	acessnsr_mgmt_ops = {
	acessnsr_usage_ind,
	udi_enumerate_no_children,
	acessnsr_devmgmt_req,
	acessnsr_final_cleanup_req
};
udi_ubit8_t	acessnsr_mgmt_ops_flags[4] = {0,0,0,0};

udi_primary_init_t	acessnsr_pri_init = {
	.mgmt_ops = &acessnsr_mgmt_ops,
	.mgmt_op_flags = acessnsr_mgmt_ops_flags,
	.mgmt_scratch_requirement = 0,
	.enumeration_attr_list_length = 4,
	.rdata_size = sizeof(acessnsr_rdata_t),
	.child_data_size = 0,
	.per_parent_paths = 0
};

udi_nsr_ctrl_ops_t	acessnsr_ctrl_ops = {
	acessnsr_ctrl_channel_event_ind,
	acessnsr_ctrl_bind_ack,
	acessnsr_ctrl_unbind_ack,
	acessnsr_ctrl_enable_ack,
	acessnsr_ctrl_ctrl_ack,
	acessnsr_ctrl_info_ack,
	acessnsr_ctrl_status_ind
};
udi_ubit8_t	acessnsr_ctrl_op_flags[7] = {0};

udi_nsr_tx_ops_t	acessnsr_tx_ops = {
	acessnsr_tx_channel_event_ind,
	acessnsr_tx_rdy
};
udi_ubit8_t	acessnsr_tx_ops_flags[2] = {0};

udi_nsr_rx_ops_t	acessnsr_rx_ops = {
	acessnsr_rx_channel_event_ind,
	acessnsr_rx_ind,
	acessnsr_rx_exp_ind
};
udi_ubit8_t	acessnsr_rx_ops_flags[3] = {0};

udi_ops_init_t	acessnsr_ops_list[] = {
	{
		ACESSNSR_OPS_CTRL, ACESSNSR_META_NIC, UDI_NSR_CTRL_OPS_NUM,
		0, (udi_ops_vector_t*)&acessnsr_ctrl_ops, acessnsr_ctrl_op_flags
	},
	{
		ACESSNSR_OPS_TX, ACESSNSR_META_NIC, UDI_NSR_TX_OPS_NUM,
		0, (udi_ops_vector_t*)&acessnsr_tx_ops, acessnsr_tx_ops_flags
	},
	{
		ACESSNSR_OPS_RX, ACESSNSR_META_NIC, UDI_NSR_RX_OPS_NUM,
		0, (udi_ops_vector_t*)&acessnsr_rx_ops, acessnsr_rx_ops_flags
	},
	{0}
};
udi_cb_init_t	acessnsr_cb_init_list[] = {
	{ACESSNSR_CB_CTRL, ACESSNSR_META_NIC, UDI_NIC_BIND_CB_NUM, 0, 0,NULL},
	{0}
};
const udi_init_t	acessnsr_init = {
	.primary_init_info = &acessnsr_pri_init,
	.ops_init_list = acessnsr_ops_list,
	.cb_init_list = acessnsr_cb_init_list,
};
const char	acessnsr_udiprops[] = 
	"properties_version 0x101\0"
	"message 1 Acess2 Kernel\0"
	"message 2 John Hodge (acess@mutabah.net)\0"
	"message 3 Acess2 NSR\0"
	"supplier 1\0"
	"contact 2\0"
	"name 3\0"
	"module acess_nsr\0"
	"shortname acessnsr\0"
	"requires udi 0x101\0"
	"requires udi_nic 0x101\0"
	"meta 1 udi_nic\0"
	"message 101 Ethernet Adapter\0"
	"device 101 1 if_media string eth\0"
	"parent_bind_ops 1 0 1 1\0"
	"\0";
size_t	acessnsr_udiprops_size = sizeof(acessnsr_udiprops);

