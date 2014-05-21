/*
 * UDI Ne2000 NIC Driver
 * By John Hodge (thePowersGang)
 *
 * ne2000_common.h
 * - Shared definitions
 */
#ifndef _NE2000_COMMON_H_
#define _NE2000_COMMON_H_

#define UDI_VERSION	0x101
#define UDI_PHYSIO_VERSION	0x101
#define UDI_NIC_VERSION	0x101

#include <udi.h>
#include <udi_physio.h>
#include <udi_nic.h>

#include "ne2000_hw.h"

#define ARRAY_SIZEOF(arr)	(sizeof(arr)/sizeof(arr[0]))

enum {
	NE2K_PIO_RESET,
	NE2K_PIO_ENABLE,
	NE2K_PIO_RX,
	NE2K_PIO_IRQACK,
	NE2K_PIO_TX,
	N_NE2K_PIO
};

typedef struct
{
	udi_init_context_t	init_context;

	udi_cb_t	*active_cb;

	udi_intr_attach_cb_t	*intr_attach_cb;

	struct {
		udi_index_t	pio_index;
		udi_index_t	n_intr_event_cb;
	
		udi_index_t	rx_chan_index;
	} init;
	
	udi_pio_handle_t	pio_handles[N_NE2K_PIO];
	udi_channel_t	interrupt_channel;
	udi_channel_t	rx_channel;
	udi_channel_t	tx_channel;
	
	udi_nic_rx_cb_t	*rx_next_cb;
	udi_nic_rx_cb_t	*rx_last_cb;
	udi_ubit8_t	rx_next_page;
	
	udi_ubit8_t	macaddr[6];
} ne2k_rdata_t;

// === MACROS ===
/* Copied from http://projectudi.cvs.sourceforge.net/viewvc/projectudi/udiref/driver/udi_dpt/udi_dpt.h */
#define DPT_SET_ATTR_BOOLEAN(attr, name, val)	\
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_BOOLEAN; \
		(attr)->attr_length = sizeof(udi_boolean_t); \
		UDI_ATTR32_SET((attr)->attr_value, (val))

#define DPT_SET_ATTR32(attr, name, val)	\
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_UBIT32; \
		(attr)->attr_length = sizeof(udi_ubit32_t); \
		UDI_ATTR32_SET((attr)->attr_value, (val))

#define DPT_SET_ATTR_ARRAY8(attr, name, val, len) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_ARRAY8; \
		(attr)->attr_length = (len); \
		udi_memcpy((attr)->attr_value, (val), (len))

#define DPT_SET_ATTR_STRING(attr, name, val, len) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_STRING; \
		(attr)->attr_length = (len); \
		udi_strncpy_rtrim((char *)(attr)->attr_value, (val), (len))
#define NE2K_SET_ATTR_STRFMT(attr, name, maxlen, fmt, v...) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_STRING; \
		(attr)->attr_length = udi_snprintf((char *)(attr)->attr_value, (maxlen), (fmt) ,## v )

extern udi_channel_event_ind_op_t	ne2k_bus_dev_channel_event_ind;
extern udi_bus_bind_ack_op_t	ne2k_bus_dev_bus_bind_ack;
extern udi_pio_map_call_t	ne2k_bus_dev_bind__pio_map;
extern udi_channel_spawn_call_t	ne2k_bus_dev_bind__intr_chanel;
extern udi_cb_alloc_call_t	ne2k_bus_dev_bind__intr_attach;
extern udi_bus_unbind_ack_op_t	ne2k_bus_dev_bus_unbind_ack;
extern udi_intr_attach_ack_op_t	ne2k_bus_dev_intr_attach_ack;
extern udi_cb_alloc_call_t	ne2k_bus_dev_bind__intr_event_cb;
extern udi_pio_trans_call_t	ne2k_bus_dev_bind__card_reset;
extern udi_intr_detach_ack_op_t	ne2k_bus_dev_intr_detach_ack;

extern udi_channel_event_ind_op_t	ne2k_nd_ctrl_channel_event_ind;
extern udi_nd_bind_req_op_t	ne2k_nd_ctrl_bind_req;
extern udi_channel_spawn_call_t	ne2k_nd_ctrl_bind__tx_chan_ok;
extern udi_channel_spawn_call_t	ne2k_nd_ctrl_bind__rx_chan_ok;
extern udi_nd_unbind_req_op_t	ne2k_nd_ctrl_unbind_req;
extern udi_nd_enable_req_op_t	ne2k_nd_ctrl_enable_req;
extern udi_pio_trans_call_t	ne2k_nd_ctrl_enable_req__trans_done;
extern udi_nd_disable_req_op_t	ne2k_nd_ctrl_disable_req;
extern udi_nd_ctrl_req_op_t	ne2k_nd_ctrl_ctrl_req;
extern udi_nd_info_req_op_t	ne2k_nd_ctrl_info_req;

extern udi_channel_event_ind_op_t	ne2k_nd_tx_channel_event_ind;
extern udi_nd_tx_req_op_t	ne2k_nd_tx_tx_req;
extern udi_nd_exp_tx_req_op_t	ne2k_nd_tx_exp_tx_req;

extern udi_channel_event_ind_op_t	ne2k_nd_rx_channel_event_ind;
extern udi_nd_rx_rdy_op_t	ne2k_nd_rx_rx_rdy;

extern udi_channel_event_ind_op_t	ne2k_bus_irq_channel_event_ind;
extern udi_intr_event_ind_op_t	ne2k_bus_irq_intr_event_ind;

extern void	ne2k_intr__rx_ok(udi_cb_t *gcb);

#endif

