/**
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * udi_lib/udi_nic.c
 * - Network Interface metalanguage
 */
#define DEBUG	1
#include <udi.h>
#include <udi_nic.h>
#include <acess.h>	// for EXPORT
#include <udi_internal.h>


extern udi_mei_init_t	udi_mei_info__nic;
#define udi_mei_info	udi_mei_info__nic

// === EXPORTS ===
EXPORT(udi_nd_bind_req);
EXPORT(udi_nsr_bind_ack);
EXPORT(udi_nd_unbind_req);
EXPORT(udi_nsr_unbind_ack);
EXPORT(udi_nd_enable_req);
EXPORT(udi_nsr_enable_ack);
EXPORT(udi_nd_disable_req);
EXPORT(udi_nd_ctrl_req);
EXPORT(udi_nsr_ctrl_ack);
EXPORT(udi_nsr_status_ind);
EXPORT(udi_nd_info_req);
EXPORT(udi_nsr_info_ack);
// - TX
EXPORT(udi_nsr_tx_rdy);
EXPORT(udi_nd_tx_req);
EXPORT(udi_nd_exp_tx_req);
// - RX
EXPORT(udi_nsr_rx_ind);
EXPORT(udi_nsr_exp_rx_ind);
EXPORT(udi_nd_rx_rdy);

// === GLOBALS ===
tUDI_MetaLang	cMetaLang_NIC = {
	"udi_nic",
	8,
	{
		{0},
		{sizeof(udi_nic_cb_t), NULL},
		{sizeof(udi_nic_bind_cb_t), NULL},
		{sizeof(udi_nic_ctrl_cb_t), NULL},
		{sizeof(udi_nic_status_cb_t), NULL},
		{sizeof(udi_nic_info_cb_t), NULL},
		{sizeof(udi_nic_tx_cb_t), NULL},
		{sizeof(udi_nic_rx_cb_t), NULL},
	}
};

// === CODE ===
// --- Control Ops ---
UDI_MEI_STUBS(udi_nd_bind_req, udi_nic_bind_cb_t,
	2,
	(tx_chan_index,  rx_chan_index),
	(udi_index_t,    udi_index_t),
	(UDI_VA_INDEX_T, UDI_VA_INDEX_T),
	UDI_ND_CTRL_OPS_NUM, 1)
udi_layout_t	_udi_nd_bind_req_marshal_layout[] = { UDI_DL_INDEX_T, UDI_DL_INDEX_T, UDI_DL_END };
UDI_MEI_STUBS(udi_nsr_bind_ack, udi_nic_bind_cb_t,
	1, (status), (udi_status_t), (UDI_VA_STATUS_T),
	UDI_NSR_CTRL_OPS_NUM, 1)
udi_layout_t	_udi_nsr_bind_ack_marshal_layout[] = { UDI_DL_STATUS_T, UDI_DL_END };

void udi_nd_unbind_req(udi_nic_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nsr_unbind_ack(udi_nic_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}

void udi_nd_enable_req(udi_nic_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nsr_enable_ack(udi_nic_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}

void udi_nd_disable_req(udi_nic_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nd_ctrl_req(udi_nic_ctrl_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nsr_ctrl_ack(udi_nic_ctrl_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}

void udi_nsr_status_ind(udi_nic_status_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nd_info_req(udi_nic_info_cb_t *cb, udi_boolean_t reset_statistics)
{
	UNIMPLEMENTED();
}

void udi_nsr_info_ack(udi_nic_info_cb_t *cb)
{
	UNIMPLEMENTED();
}

// --- TX ---
void udi_nsr_tx_rdy(udi_nic_tx_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nd_tx_req(udi_nic_tx_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nd_exp_tx_req(udi_nic_tx_cb_t *cb)
{
	UNIMPLEMENTED();
}

// --- RX ---
UDI_MEI_STUBS(udi_nsr_rx_ind,     udi_nic_rx_cb_t, 0, (), (), (), UDI_NSR_RX_OPS_NUM, 1)
udi_layout_t	_udi_nsr_rx_ind_marshal_layout[] = { UDI_DL_END };
UDI_MEI_STUBS(udi_nsr_exp_rx_ind, udi_nic_rx_cb_t, 0, (), (), (), UDI_NSR_RX_OPS_NUM, 2)
udi_layout_t	_udi_nsr_exp_rx_ind_marshal_layout[] = { UDI_DL_END };
UDI_MEI_STUBS(udi_nd_rx_rdy, udi_nic_rx_cb_t, 0, (), (), (), UDI_ND_RX_OPS_NUM, 1)
udi_layout_t	_udi_nd_rx_rdy_marshal_layout[] = { UDI_DL_END };

#define UDI__OPS_NUM	0
#define MEI_OPINFO(name,cat,flags,cbtype,rsp_ops,rsp_idx,err_ops,err_idx)	\
	{#name, UDI_MEI_OPCAT_##cat,flags,UDI_##cbtype##_CB_NUM, \
		UDI_##rsp_ops##_OPS_NUM,rsp_idx,UDI_##err_ops##_OPS_NUM,err_idx, \
		name##_direct, name##_backend, _##cbtype##_cb_layout, _##name##_marshal_layout }

udi_layout_t	_NIC_BIND_cb_layout[] = {
	UDI_DL_UBIT8_T,	// media_type
	UDI_DL_UBIT32_T,	// min_pdu_size
	UDI_DL_UBIT32_T,
	UDI_DL_UBIT32_T,
	UDI_DL_UBIT32_T,
	UDI_DL_UBIT8_T,	// max_perfect_multicast
	UDI_DL_UBIT8_T,
	UDI_DL_UBIT8_T,	// mac_addr_len
	UDI_DL_ARRAY,	// mac_addr
		UDI_NIC_MAC_ADDRESS_SIZE,
		UDI_DL_UBIT8_T,
		UDI_DL_END,
	UDI_DL_END
};
udi_layout_t	_NIC_RX_cb_layout[] = {
	UDI_DL_CB,	// chain
	UDI_DL_BUF, 0, 0, 0,	// rx_buf
	UDI_DL_UBIT8_T,	// rx_status
	UDI_DL_UBIT8_T,	// addr_match
	UDI_DL_UBIT8_T,	// rx_valid
	UDI_DL_END
};

udi_mei_op_template_t	udi_mei_info__nic__nd_ctrl_ops[] = {
	MEI_OPINFO(udi_nd_bind_req, REQ, 0, NIC_BIND, NSR_CTRL,1, ,0),
	{0} 
};
udi_mei_op_template_t	udi_mei_info__nic__nsr_ctrl_ops[] = {
	MEI_OPINFO(udi_nsr_bind_ack, ACK, 0, NIC_BIND, ,0, ,0),
	{0}
};
udi_mei_op_template_t	udi_mei_info__nic__nd_tx_ops[] = {
	{0}
};
udi_mei_op_template_t	udi_mei_info__nic__nsr_tx_ops[] = {
	{0}
};
udi_mei_op_template_t	udi_mei_info__nic__nd_rx_ops[] = {
	MEI_OPINFO(udi_nd_rx_rdy, RDY, 0, NIC_RX, ,0, ,0),
	{0}
};
udi_mei_op_template_t	udi_mei_info__nic__nsr_rx_ops[] = {
	MEI_OPINFO(udi_nsr_rx_ind, IND, 0, NIC_RX, ND_RX,1, ,0),
	MEI_OPINFO(udi_nsr_exp_rx_ind, IND, 0, NIC_RX, ND_RX,1, ,0),
	{0}
};

udi_mei_ops_vec_template_t	udi_mei_info__nic_ops[] = {
	{UDI_ND_CTRL_OPS_NUM, UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_BIND|UDI_MEI_REL_INITIATOR, udi_mei_info__nic__nd_ctrl_ops},
	{UDI_NSR_CTRL_OPS_NUM, UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_BIND, udi_mei_info__nic__nsr_ctrl_ops},
	{UDI_ND_TX_OPS_NUM, UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_INITIATOR, udi_mei_info__nic__nd_tx_ops},
	{UDI_NSR_TX_OPS_NUM, UDI_MEI_REL_EXTERNAL, udi_mei_info__nic__nsr_tx_ops},
	{UDI_ND_RX_OPS_NUM, UDI_MEI_REL_EXTERNAL|UDI_MEI_REL_INITIATOR, udi_mei_info__nic__nd_rx_ops},
	{UDI_NSR_RX_OPS_NUM, UDI_MEI_REL_EXTERNAL, udi_mei_info__nic__nsr_rx_ops},
	{0}
};
udi_mei_init_t	udi_mei_info__nic = {
	udi_mei_info__nic_ops,
	NULL
};
