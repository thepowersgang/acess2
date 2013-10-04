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

enum {
	NE2K_META_BUS = 1,
	NE2K_META_NIC,
};
enum {
	NE2K_OPS_DEV = 1,
	NE2K_OPS_CTRL,
	NE2K_OPS_TX,
	NE2K_OPS_RX,
	NE2K_OPS_IRQ,
};
enum {
	NE2K_CB_BUS_BIND = 1,
	NE2K_CB_INTR,
	NE2K_CB_INTR_EVENT,
};

#define NE2K_NUM_INTR_EVENT_CBS	4

// === GLOBALS ===
#define PIO_op_RI(op, reg, sz, val)	{UDI_PIO_##op+UDI_PIO_DIRECT+UDI_PIO_##reg, UDI_PIO_##sz##BYTE, val}
#define PIO_MOV_RI1(reg, val)	PIO_op_RI(LOAD_IMM, reg, 1, val)
#define PIO_OUT_RI1(reg, ofs)	PIO_op_RI(OUT, reg, 1, ofs)
#define PIO_IN_RI1(reg, ofs)	PIO_op_RI(IN, reg, 1, ofs)
// --- Programmed IO ---
#include "ne2000_pio.h"

// === CODE ===
// --- Management
void ne2k_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	udi_usage_res(cb);
}
void ne2k_enumerate_req(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level)
{
	ne2k_rdata_t	*rdata = UDI_GCB(cb)->context;
	udi_instance_attr_list_t *attr_list = cb->attr_list;
	
	switch(enumeration_level)
	{
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:
		// Emit the ND binding
		DPT_SET_ATTR32(attr_list, "if_num", 0);
		attr_list ++;
		DPT_SET_ATTR_STRING(attr_list, "if_media", "eth", 3);
		attr_list ++;
		NE2K_SET_ATTR_STRFMT(attr_list, "identifier", 2*6+1, "%2X%2X%2X%2X%2X%2X",
			rdata->macaddr[0], rdata->macaddr[1], rdata->macaddr[2],
			rdata->macaddr[3], rdata->macaddr[4], rdata->macaddr[5] );
		attr_list ++;
		cb->attr_valid_length = attr_list - cb->attr_list;
		udi_enumerate_ack(cb, UDI_ENUMERATE_OK, NE2K_OPS_CTRL);
		break;
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
	switch(cb->event)
	{
	case UDI_CHANNEL_CLOSED:
		break;
	case UDI_CHANNEL_BOUND: {
		udi_bus_bind_cb_t *bus_bind_cb = UDI_MCB(cb->params.parent_bound.bind_cb, udi_bus_bind_cb_t);
		udi_bus_bind_req( bus_bind_cb );
		// continue at ne2k_bus_dev_bus_bind_ack
		return; }
	}
}
void ne2k_bus_dev_bus_bind_ack(udi_bus_bind_cb_t *cb,
	udi_dma_constraints_t dma_constraints, udi_ubit8_t perferred_endianness, udi_status_t status)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	ne2k_rdata_t	*rdata = gcb->context;
	rdata->active_cb = gcb;
	
	// Set up PIO handles
	rdata->init.pio_index = -1;
	ne2k_bus_dev_bind__pio_map(gcb, UDI_NULL_PIO_HANDLE);
}
void ne2k_bus_dev_bind__pio_map(udi_cb_t *gcb, udi_pio_handle_t new_pio_handle)
{
	ne2k_rdata_t	*rdata = gcb->context;
	
	if( rdata->init.pio_index != -1 )
	{
		rdata->pio_handles[rdata->init.pio_index] = new_pio_handle;
	}
	rdata->init.pio_index ++;
	if( rdata->init.pio_index < NE2K_NUM_PIO_OPS )
	{
		udi_pio_map(ne2k_bus_dev_bind__pio_map, gcb,
			UDI_PCI_BAR_0, 0, 0x20,
			ne2k_pio_ops[rdata->init.pio_index].trans_list,
			ne2k_pio_ops[rdata->init.pio_index].list_length,
			UDI_PIO_LITTLE_ENDIAN, 0, 0
			);
		return ;
	}
	
	// Next: Bind interrupt
	udi_channel_spawn(ne2k_bus_dev_bind__intr_chanel, gcb, gcb->channel,
		0, NE2K_OPS_IRQ, rdata);
}
void ne2k_bus_dev_bind__intr_chanel(udi_cb_t *gcb, udi_channel_t new_channel)
{
	ne2k_rdata_t	*rdata = gcb->context;
	
	rdata->interrupt_channel = new_channel;
	
	udi_cb_alloc(ne2k_bus_dev_bind__intr_attach, gcb, NE2K_CB_INTR, gcb->channel);
}
void ne2k_bus_dev_bind__intr_attach(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	ne2k_rdata_t	*rdata = gcb->context;
	udi_intr_attach_cb_t	*intr_cb = UDI_MCB(new_cb, udi_intr_attach_cb_t);
	intr_cb->interrupt_idx = 0;
	intr_cb->min_event_pend = 2;
	intr_cb->preprocessing_handle = rdata->pio_handles[NE2K_PIO_IRQACK];
	udi_intr_attach_req(intr_cb);
	// continued in ne2k_bus_dev_intr_attach_ack
}
void ne2k_bus_dev_bus_unbind_ack(udi_bus_bind_cb_t *cb)
{
}
void ne2k_bus_dev_intr_attach_ack(udi_intr_attach_cb_t *intr_attach_cb, udi_status_t status)
{
	udi_cb_t	*gcb = UDI_GCB(intr_attach_cb);
	ne2k_rdata_t	*rdata = gcb->context;
	// continuing from ne2k_bus_dev_bind__intr_attach
	if( status != UDI_OK ) {
		// TODO: Error
		udi_cb_free( UDI_GCB(intr_attach_cb) );
		return ;
	}
	
	rdata->intr_attach_cb = intr_attach_cb;
	
	rdata->init.n_intr_event_cb = 0;
	udi_cb_alloc(ne2k_bus_dev_bind__intr_event_cb, gcb, NE2K_CB_INTR_EVENT, rdata->interrupt_channel);
	// V V V V
}
void ne2k_bus_dev_bind__intr_event_cb(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	ne2k_rdata_t	*rdata = gcb->context;
	
	udi_intr_event_cb_t *intr_event_cb = UDI_MCB(new_cb, udi_intr_event_cb_t);
	udi_intr_event_rdy(intr_event_cb);
	rdata->init.n_intr_event_cb ++;
	
	if( rdata->init.n_intr_event_cb < NE2K_NUM_INTR_EVENT_CBS )
	{
		udi_cb_alloc(ne2k_bus_dev_bind__intr_event_cb, gcb,
			NE2K_CB_INTR_EVENT, rdata->interrupt_channel);
		// A A A A
		return ;
	}

	udi_pio_trans(ne2k_bus_dev_bind__card_reset, gcb,
		rdata->pio_handles[NE2K_PIO_RESET], 0, NULL, &rdata->macaddr);
	// V V V V
}

void ne2k_bus_dev_bind__card_reset(udi_cb_t *gcb, udi_buf_t *new_buf, udi_status_t status, udi_ubit16_t result)
{	
	ne2k_rdata_t	*rdata = gcb->context;
	// Done! (Finally)
	udi_channel_event_complete( UDI_MCB(rdata->active_cb, udi_channel_event_cb_t), UDI_OK );
	// = = = =
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
	udi_cb_t	*gcb = UDI_GCB(cb);
	ne2k_rdata_t	*rdata = gcb->context;
	rdata->init.rx_chan_index = rx_chan_index;
	udi_channel_spawn(ne2k_nd_ctrl_bind__tx_chan_ok, gcb, gcb->channel, tx_chan_index, NE2K_OPS_TX, rdata);
	// V V V V
}
void ne2k_nd_ctrl_bind__tx_chan_ok(udi_cb_t *gcb, udi_channel_t new_channel)
{
	ne2k_rdata_t	*rdata = gcb->context;
	rdata->tx_channel = new_channel;
	udi_channel_spawn(ne2k_nd_ctrl_bind__rx_chan_ok, gcb, gcb->channel,
		rdata->init.rx_chan_index, NE2K_OPS_RX, rdata);
	// V V V V
}
void ne2k_nd_ctrl_bind__rx_chan_ok(udi_cb_t *cb, udi_channel_t new_channel)
{
	ne2k_rdata_t	*rdata = cb->context;
	rdata->rx_channel = new_channel;
	udi_nsr_bind_ack( UDI_MCB(cb, udi_nic_bind_cb_t), UDI_OK );
	// = = = =
}
void ne2k_nd_ctrl_unbind_req(udi_nic_cb_t *cb)
{
}
void ne2k_nd_ctrl_enable_req(udi_nic_cb_t *cb)
{
	// Enable
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
// --- IRQ
void ne2k_bus_irq_channel_event_ind(udi_channel_event_cb_t *cb)
{
}
void ne2k_bus_irq_intr_event_ind(udi_intr_event_cb_t *cb, udi_ubit8_t flags)
{
	if( cb->intr_result & 0x01 )
	{
		ne2k_intr__rx_ok( UDI_GCB(cb) );
	}
	// TODO: TX IRQs
	udi_intr_event_rdy(cb);
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
udi_nd_tx_ops_t	ne2k_nd_tx_ops = {
	ne2k_nd_tx_channel_event_ind,
	ne2k_nd_tx_tx_req,
	ne2k_nd_tx_exp_tx_req
};
udi_ubit8_t	ne2k_nd_tx_ops_flags[3] = {0};
udi_nd_rx_ops_t	ne2k_nd_rx_ops = {
	ne2k_nd_rx_channel_event_ind,
	ne2k_nd_rx_rx_rdy
};
udi_ubit8_t	ne2k_nd_rx_ops_flags[2] = {0};
udi_intr_handler_ops_t	ne2k_bus_irq_ops = {
	ne2k_bus_irq_channel_event_ind,
	ne2k_bus_irq_intr_event_ind
};
udi_ubit8_t	ne2k_bus_irq_ops_flags[2] = {0};

udi_primary_init_t	ne2k_pri_init = {
	.mgmt_ops = &ne2k_mgmt_ops,
	.mgmt_op_flags = ne2k_mgmt_op_flags,
	.mgmt_scratch_requirement = 0,
	.enumeration_attr_list_length = 4,
	.rdata_size = sizeof(ne2k_rdata_t),
	.child_data_size = 0,
	.per_parent_paths = 0
};
udi_ops_init_t	ne2k_ops_list[] = {
	{
		NE2K_OPS_DEV, NE2K_META_BUS, UDI_BUS_DEVICE_OPS_NUM,
		0,
		(udi_ops_vector_t*)&ne2k_bus_dev_ops,
		ne2k_bus_dev_ops_flags
	},
	{
		NE2K_OPS_CTRL, NE2K_META_NIC, UDI_ND_CTRL_OPS_NUM,
		0,
		(udi_ops_vector_t*)&ne2k_nd_ctrl_ops,
		ne2k_nd_ctrl_ops_flags
	},
	{
		NE2K_OPS_TX, NE2K_META_NIC, UDI_ND_TX_OPS_NUM,
		0,
		(udi_ops_vector_t*)&ne2k_nd_tx_ops,
		ne2k_nd_tx_ops_flags
	},
	{
		NE2K_OPS_RX, NE2K_META_NIC, UDI_ND_RX_OPS_NUM,
		0,
		(udi_ops_vector_t*)&ne2k_nd_rx_ops,
		ne2k_nd_rx_ops_flags
	},
	{
		NE2K_OPS_IRQ, NE2K_META_BUS, UDI_BUS_INTR_HANDLER_OPS_NUM,
		0,
		(udi_ops_vector_t*)&ne2k_bus_irq_ops,
		ne2k_bus_irq_ops_flags
	},
	{0}
};
udi_cb_init_t ne2k_cb_init_list[] = {
	// Parent bind
	{NE2K_CB_BUS_BIND, NE2K_META_BUS, UDI_BUS_BIND_CB_NUM, 0, 0,NULL},
	{0}
};
// TODO: cb_init_list
const udi_init_t	udi_init_info = {
	.primary_init_info = &ne2k_pri_init,
	.ops_init_list = ne2k_ops_list,
	.cb_init_list = ne2k_cb_init_list,
};
