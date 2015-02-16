/*
 * Acess2 UDI Environment (NIC Bindings)
 * - By John Hodge (thePowersGang)
 *
 * udi_nic.h
 * - NIC Bindings core
 */
#ifndef _UDI_NIC_H_
#define _UDI_NIC_H_

#ifndef UDI_NIC_VERSION
# error "UDI_NIC_VERSION must be defined"
#endif

// === CBs ===
#define UDI_NIC_STD_CB_NUM	1
#define UDI_NIC_BIND_CB_NUM	2
#define UDI_NIC_CTRL_CB_NUM	3
#define UDI_NIC_STATUS_CB_NUM	4
#define UDI_NIC_INFO_CB_NUM	5
#define UDI_NIC_TX_CB_NUM	6
#define UDI_NIC_RX_CB_NUM	7

typedef struct {
	udi_cb_t	gcb;
} udi_nic_cb_t;

#define UDI_NIC_MAC_ADDRESS_SIZE	20
// Media type
#define UDI_NIC_ETHER   	0
#define UDI_NIC_TOKEN   	1
#define UDI_NIC_FASTETHER	2
#define UDI_NIC_GIGETHER	3
#define UDI_NIC_VGANYLAN	4
#define UDI_NIC_FDDI    	5
#define UDI_NIC_ATM     	6
#define UDI_NIC_FC      	7
#define UDI_NIC_MISCMEDIA	0xff
// capabilities
#define UDI_NIC_CAP_TX_IP_CKSUM 	(1U<<0)
#define UDI_NIC_CAP_TX_TCP_CKSUM	(1U<<1)
#define UDI_NIC_CAP_TX_UDP_CKSUM	(1U<<2)
#define UDI_NIC_CAP_MCAST_LOOPBK	(1U<<3)
#define UDI_NIC_CAP_BCAST_LOOPBK	(1U<<4)
// capabilities (requests)
#define UDI_NIC_CAP_USE_TX_CKSUM	(1U<<30)
#define UDI_NIC_CAP_USE_RX_CKSUM	(1U<<31)
typedef struct {
	udi_cb_t	gcb;
	udi_ubit8_t	media_type;
	udi_ubit32_t	min_pdu_size;
	udi_ubit32_t	max_pdu_size;
	udi_ubit32_t	rx_hw_threshold;
	udi_ubit32_t	capabilities;
	udi_ubit8_t	max_perfect_multicast;
	udi_ubit8_t	max_total_multicast;
	udi_ubit8_t	mac_addr_len;
	udi_ubit8_t	mac_addr[UDI_NIC_MAC_ADDRESS_SIZE];
} udi_nic_bind_cb_t;

// commands
#define UDI_NIC_ADD_MULTI	1
#define UDI_NIC_DEL_MULTI	2
#define UDI_NIC_ALLMULTI_ON	3
#define UDI_NIC_ALLMULTI_OFF	4
#define UDI_NIC_GET_CURR_MAC	5
#define UDI_NIC_SET_CURR_MAC	6
#define UDI_NIC_GET_FACT_MAC	7
#define UDI_NIC_PROMISC_ON	8
#define UDI_NIC_PROMISC_OFF	9
#define UDI_NIC_HW_RESET	10
#define UDI_NIC_BAD_RXPKT	11
typedef struct {
	udi_cb_t	gcb;
	udi_ubit8_t	command;
	udi_ubit32_t	indicator;
	udi_buf_t	*data_buf;
} udi_nic_ctrl_cb_t;

// event
#define UDI_NIC_LINK_DOWN	0
#define UDI_NIC_LINK_UP 	1
#define UDI_NIC_LINK_RESET	2
typedef struct {
	udi_cb_t	gcb;
	udi_ubit8_t	event;
} udi_nic_status_cb_t;

typedef struct {
	udi_cb_t gcb;
	udi_boolean_t	interface_is_active;
	udi_boolean_t	link_is_active;
	udi_boolean_t	is_full_duplex;
	udi_ubit32_t	link_mbps;
	udi_ubit32_t	link_bps;
	udi_ubit32_t	tx_packets;
	udi_ubit32_t	rx_packets;
	udi_ubit32_t	tx_errors;
	udi_ubit32_t	rx_errors;
	udi_ubit32_t	tx_discards;
	udi_ubit32_t	rx_discards;
	udi_ubit32_t	tx_underrun;
	udi_ubit32_t	rx_overrun;
	udi_ubit32_t	collisions;
} udi_nic_info_cb_t;

typedef struct udi_nic_tx_cb_s {
	udi_cb_t	gcb;
	struct udi_nic_tx_cb_s	*chain;
	udi_buf_t	*tx_buf;
	udi_boolean_t	completion_urgent;
} udi_nic_tx_cb_t;

// rx_status
#define UDI_NIC_RX_BADCKSUM	(1U<<0)
#define UDI_NIC_RX_UNDERRUN	(1U<<1)
#define UDI_NIC_RX_OVERRUN	(1U<<2)
#define UDI_NIC_RX_DRIBBLE	(1U<<3)
#define UDI_NIC_RX_FRAME_ERR	(1U<<4)
#define UDI_NIC_RX_MAC_ERR	(1U<<5)
#define UDI_NIC_RX_OTHER_ERR	(1U<<7)
// addr_match
#define UDI_NIC_RX_UNKNOWN	0
#define UDI_NIC_RX_EXACT	1
#define UDI_NIC_RX_HASH 	2
#define UDI_NIC_RX_BROADCAST	3
// rx_valid
#define UDI_NIC_RX_GOOD_IP_CKSUM	(1U<<0)
#define UDI_NIC_RX_GOOD_TCP_CKSUM	(1U<<1)
#define UDI_NIC_RX_GOOD_UDP_CKSUM	(1U<<2)
typedef struct udi_nic_rx_cb_s {
	udi_cb_t	gcb;
	struct udi_nic_rx_cb_s	*chain;
	udi_buf_t	*rx_buf;
	udi_ubit8_t	rx_status;
	udi_ubit8_t	addr_match;
	udi_ubit8_t	rx_valid;
} udi_nic_rx_cb_t;

// === Function Types ===
// - Control
typedef void	udi_nd_bind_req_op_t(udi_nic_bind_cb_t *cb, udi_index_t tx_chan_index, udi_index_t rx_chan_index);
typedef void	udi_nsr_bind_ack_op_t(udi_nic_bind_cb_t *cb, udi_status_t status);
typedef void	udi_nd_unbind_req_op_t(udi_nic_cb_t *cb);
typedef void	udi_nsr_unbind_ack_op_t(udi_nic_cb_t *cb, udi_status_t status);
typedef void	udi_nd_enable_req_op_t(udi_nic_cb_t *cb);
typedef void	udi_nsr_enable_ack_op_t(udi_nic_cb_t *cb, udi_status_t status);
typedef void	udi_nd_disable_req_op_t(udi_nic_cb_t *cb);
typedef void	udi_nsr_disable_ack_op_t(udi_nic_cb_t *cb, udi_status_t status);
typedef void	udi_nd_ctrl_req_op_t(udi_nic_ctrl_cb_t *cb);
typedef void	udi_nsr_ctrl_ack_op_t(udi_nic_ctrl_cb_t *cb, udi_status_t status);
typedef void	udi_nsr_status_ind_op_t(udi_nic_status_cb_t *cb);
typedef void	udi_nd_info_req_op_t(udi_nic_info_cb_t *cb, udi_boolean_t reset_statistics);
typedef void	udi_nsr_info_ack_op_t(udi_nic_info_cb_t *cb);
// - TX
typedef void	udi_nsr_tx_rdy_op_t(udi_nic_tx_cb_t *cb);
typedef void	udi_nd_tx_req_op_t(udi_nic_tx_cb_t *cb);
typedef void	udi_nd_exp_tx_req_op_t(udi_nic_tx_cb_t *cb);
// - RX
typedef void	udi_nsr_rx_ind_op_t(udi_nic_rx_cb_t *cb);
typedef void	udi_nsr_exp_rx_ind_op_t(udi_nic_rx_cb_t *cb);
typedef void	udi_nd_rx_rdy_op_t(udi_nic_rx_cb_t *cb);

// === Functions ===
// - Control
extern udi_nd_bind_req_op_t	udi_nd_bind_req;
extern udi_nsr_bind_ack_op_t	udi_nsr_bind_ack;
extern udi_nd_unbind_req_op_t udi_nd_unbind_req;
extern udi_nsr_unbind_ack_op_t udi_nsr_unbind_ack;
extern udi_nd_enable_req_op_t udi_nd_enable_req;
extern udi_nsr_enable_ack_op_t udi_nsr_enable_ack;
extern udi_nd_disable_req_op_t udi_nd_disable_req;
extern udi_nsr_disable_ack_op_t udi_nsr_disable_ack;
extern udi_nd_ctrl_req_op_t udi_nd_ctrl_req;
extern udi_nsr_ctrl_ack_op_t udi_nsr_ctrl_ack;
extern udi_nsr_status_ind_op_t udi_nsr_status_ind;
extern udi_nd_info_req_op_t udi_nd_info_req;
extern udi_nsr_info_ack_op_t udi_nsr_info_ack;
// - TX
extern udi_nsr_tx_rdy_op_t udi_nsr_tx_rdy;
extern udi_nd_tx_req_op_t udi_nd_tx_req;
extern udi_nd_exp_tx_req_op_t udi_nd_exp_tx_req;
// - RX
extern udi_nsr_rx_ind_op_t udi_nsr_rx_ind;
extern udi_nsr_exp_rx_ind_op_t udi_nsr_exp_rx_ind;
extern udi_nd_rx_rdy_op_t udi_nd_rx_rdy;

// === Op Lists ==
#define UDI_ND_CTRL_OPS_NUM	1
#define UDI_ND_TX_OPS_NUM	2
#define UDI_ND_RX_OPS_NUM	3
#define UDI_NSR_CTRL_OPS_NUM	4
#define UDI_NSR_TX_OPS_NUM	5
#define UDI_NSR_RX_OPS_NUM	6

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nd_bind_req_op_t	*nd_bind_req_op;
	udi_nd_unbind_req_op_t	*nd_unbind_req_op;
	udi_nd_enable_req_op_t	*nd_enable_req_op;
	udi_nd_disable_req_op_t	*nd_disable_req_op;
	udi_nd_ctrl_req_op_t	*nd_ctrl_req_op;
	udi_nd_info_req_op_t	*nd_info_req_op;
} udi_nd_ctrl_ops_t;

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nd_tx_req_op_t	*nd_tx_req_op;
	udi_nd_exp_tx_req_op_t	*nd_exp_tx_req_op;
} udi_nd_tx_ops_t;

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nd_rx_rdy_op_t	*nd_rx_rdy_op;
} udi_nd_rx_ops_t;

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nsr_bind_ack_op_t	*nsr_bind_ack_op;
	udi_nsr_unbind_ack_op_t	*nsr_unbind_ack_op;
	udi_nsr_enable_ack_op_t	*nsr_enable_ack_op;
	udi_nsr_ctrl_ack_op_t	*nsr_ctrl_ack_op;
	udi_nsr_info_ack_op_t	*nsr_info_ack_op;
	udi_nsr_status_ind_op_t	*nsr_status_ind_op;
} udi_nsr_ctrl_ops_t;

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nsr_tx_rdy_op_t	*nsr_tx_rdy_op;
} udi_nsr_tx_ops_t;

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_nsr_rx_ind_op_t	*nsr_rx_ind_op;
	udi_nsr_exp_rx_ind_op_t	*nsr_exp_rx_ind_op;
} udi_nsr_rx_ops_t;

#endif

