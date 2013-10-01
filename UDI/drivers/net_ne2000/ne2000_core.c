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

// === GLOBALS ===
#define PIO_op_RI(op, reg, sz, val)	{UDI_PIO_##op+UDI_PIO_DIRECT+UDI_PIO_##reg, UDI_PIO_##sz##BYTE, val}
#define PIO_MOV_RI1(reg, val)	PIO_op_RI(LOAD_IMM, reg, 1, val)
#define PIO_OUT_RI1(reg, ofs)	PIO_op_RI(OUT, reg, 1, ofs)
#define PIO_IN_RI1(reg, ofs)	PIO_op_RI(IN, reg, 1, ofs)
// --- Programmed IO ---
/// Ne2000 reset operation (reads MAC address too)
udi_pio_trans_t	ne2k_pio_reset[] = {
	// - Reset card
	PIO_IN_RI1(R0, NE2K_REG_RESET),
	PIO_OUT_RI1(R0, NE2K_REG_RESET),
	// While ISR bit 7 is unset, spin
	{UDI_PIO_LABEL, 0, 1},
		PIO_IN_RI1(R0, NE2K_REG_ISR),
		{UDI_PIO_AND_IMM+UDI_PIO_R0, UDI_PIO_1BYTE, 0x80},
	{UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_NZ},
	{UDI_PIO_BRANCH, 0, 1},
	// ISR = 0x80 [Clear reset]
	PIO_OUT_RI1(R0, NE2K_REG_ISR),
	// - Init pass 1
	// CMD = 0x40|0x21 [Page1, NoDMA, Stop]
	PIO_MOV_RI1(R0, 0x40|0x21),
	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	// CURR = First RX page
	PIO_MOV_RI1(R0, NE2K_RX_FIRST_PG),
	PIO_OUT_RI1(R0, NE2K_REG_CURR),
	// CMD = 0x21 [Page0, NoDMA, Stop]
	PIO_MOV_RI1(R0, 0x21),
	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	// DCR = ? [WORD, ...]
	PIO_MOV_RI1(R0, 0x49),
	PIO_OUT_RI1(R0, NE2K_REG_DCR),
	// IMR = 0 [Disable all]
	PIO_MOV_RI1(R0, 0x00),
	PIO_OUT_RI1(R0, NE2K_REG_IMR),
	// ISR = 0xFF [ACK all]
	PIO_MOV_RI1(R0, 0xFF),
	PIO_OUT_RI1(R0, NE2K_REG_ISR),
	// RCR = 0x20 [Monitor]
	PIO_MOV_RI1(R0, 0x20),
	PIO_OUT_RI1(R0, NE2K_REG_RCR),
	// TCR = 0x02 [TX Off, Loopback]
	PIO_MOV_RI1(R0, 0x02),
	PIO_OUT_RI1(R0, NE2K_REG_TCR),
	// - Read MAC address from EEPROM (24 bytes from 0)
	PIO_MOV_RI1(R0, 0),
	PIO_MOV_RI1(R1, 0),
	PIO_OUT_RI1(R0, NE2K_REG_RSAR0),
	PIO_OUT_RI1(R1, NE2K_REG_RSAR1),
	PIO_MOV_RI1(R0, 6*4),
	PIO_MOV_RI1(R1, 0),
	PIO_OUT_RI1(R0, NE2K_REG_RBCR0),
	PIO_OUT_RI1(R1, NE2K_REG_RBCR1),
	// CMD = 0x0A [Start remote DMA]
	PIO_MOV_RI1(R0, 0x0A),
	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	// Read MAC address
	PIO_MOV_RI1(R0, 0),	// - Buffer offset (incremented by 1 each iteration)
	PIO_MOV_RI1(R1, NE2K_REG_MEM),	// - Reg offset (no increment)
	PIO_MOV_RI1(R2, 6),	// - Six iterations
	{UDI_PIO_REP_IN_IND, UDI_PIO_1BYTE,
		UDI_PIO_REP_ARGS(UDI_PIO_BUF, UDI_PIO_R0, 1, UDI_PIO_R1, 0, UDI_PIO_R2)},
	// - Setup
	// PSTART = First RX page [Receive area start]
	PIO_MOV_RI1(R0, NE2K_RX_FIRST_PG),
	PIO_OUT_RI1(R0, NE2K_REG_PSTART),
	// BNRY = Last RX page - 1 [???]
	PIO_MOV_RI1(R0, NE2K_RX_LAST_PG-1),
	PIO_OUT_RI1(R0, NE2K_REG_BNRY),
	// PSTOP = Last RX page [???]
	PIO_MOV_RI1(R0, NE2K_RX_LAST_PG),
	PIO_OUT_RI1(R0, NE2K_REG_PSTOP),
	// > Clear all interrupt and set mask
	// ISR = 0xFF [ACK all]
	PIO_MOV_RI1(R0, 0xFF),
	PIO_OUT_RI1(R0, NE2K_REG_ISR),
	// IMR = 0x3F []
	PIO_MOV_RI1(R0, 0x3F),
	PIO_OUT_RI1(R0, NE2K_REG_IMR),
	// CMD = 0x22 [NoDMA, Start]
	PIO_MOV_RI1(R0, 0x22),
	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	// RCR = 0x0F [Wrap, Promisc]
	PIO_MOV_RI1(R0, 0x0F),
	PIO_OUT_RI1(R0, NE2K_REG_RCR),
	// TCR = 0x00 [Normal]
	PIO_MOV_RI1(R0, 0x00),
	PIO_OUT_RI1(R0, NE2K_REG_TCR),
	// TPSR = 0x40 [TX Start]
	PIO_MOV_RI1(R0, 0x40),
	PIO_OUT_RI1(R0, NE2K_REG_TPSR),
	// End
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0}
};
struct {
	udi_pio_trans_t	*trans_list;
	udi_ubit16_t	list_length;
	udi_ubit16_t	pio_attributes;
} ne2k_pio_ops[] = {
	{ne2k_pio_reset, ARRAY_SIZEOF(ne2k_pio_reset), 0}
};
const int NE2K_NUM_PIO_OPS = ARRAY_SIZEOF(ne2k_pio_ops);

// === CODE ===
// --- Management
void ne2k_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
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
		udi_enumerate_ack(cb, UDI_ENUMERATE_OK, 2);
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
	}
	else
	{
		// Next!
	}
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
		1, NE2K_META_BUS, UDI_BUS_DEVICE_OPS_NUM,
		0,
		(udi_ops_vector_t*)&ne2k_bus_dev_ops,
		ne2k_bus_dev_ops_flags
	},
	{
		2, NE2K_META_NIC, UDI_ND_CTRL_OPS_NUM,
		0,
		(udi_ops_vector_t*)&ne2k_nd_ctrl_ops,
		ne2k_nd_ctrl_ops_flags
	},
	{
		3, NE2K_META_NIC, UDI_ND_TX_OPS_NUM,
		0,
		(udi_ops_vector_t*)&ne2k_nd_tx_ops,
		ne2k_nd_tx_ops_flags
	},
	{
		4, NE2K_META_NIC, UDI_ND_RX_OPS_NUM,
		0,
		(udi_ops_vector_t*)&ne2k_nd_rx_ops,
		ne2k_nd_rx_ops_flags
	},
	{0}
};
const udi_init_t	udi_init_info = {
	.primary_init_info = &ne2k_pri_init,
	.ops_init_list = ne2k_ops_list
};
