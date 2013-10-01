/*
 * UDI Ne2000 NIC Driver
 * By John Hodge (thePowersGang)
 *
 * ne2000_common.h
 * - Shared definitions
 */
#ifndef _NE2000_COMMON_H_
#define _NE2000_COMMON_H_

#include <udi.h>
#include <udi_physio.h>
#include <udi_nic.h>

#include "ne2000_hw.h"

#define ARRAY_SIZEOF(arr)	(sizeof(arr)/sizeof(arr[0]))

typedef struct
{
	udi_init_context_t	init_context;
	
	udi_pio_handle_t	pio_handle;
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
#define NE2K_SET_ATTR_STRFMT(attr, name, maxlen, fmt, ...) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_STRING; \
		(attr)->attr_length = udi_snprintf((char *)(attr)->attr_value, (maxlen), (fmt) ,## __VA_LIST__ )

extern udi_channel_event_ind_op_t	ne2k_bus_dev_channel_event_ind;
extern udi_bus_bind_ack_op_t	ne2k_bus_dev_bus_bind_ack;
extern udi_bus_unbind_ack_op_t	ne2k_bus_dev_bus_unbind_ack;
extern udi_intr_attach_ack_op_t	ne2k_bus_dev_intr_attach_ack;
extern udi_intr_detach_ack_op_t	ne2k_bus_dev_intr_detach_ack;

extern udi_channel_event_ind_op_t	ne2k_nd_ctrl_channel_event_ind;
extern udi_nd_bind_req_op_t	ne2k_nd_ctrl_bind_req;
extern udi_nd_unbind_req_op_t	ne2k_nd_ctrl_unbind_req;
extern udi_nd_enable_req_op_t	ne2k_nd_ctrl_enable_req;
extern udi_nd_disable_req_op_t	ne2k_nd_ctrl_disable_req;
extern udi_nd_ctrl_req_op_t	ne2k_nd_ctrl_ctrl_req;
extern udi_nd_info_req_op_t	ne2k_nd_ctrl_info_req;

extern udi_channel_event_ind_op_t	ne2k_nd_tx_channel_event_ind;
extern udi_nd_tx_req_op_t	ne2k_nd_tx_tx_req;
extern udi_nd_exp_tx_req_op_t	ne2k_nd_tx_exp_tx_req;

extern udi_channel_event_ind_op_t	ne2k_nd_rx_channel_event_ind;
extern udi_nd_rx_rdy_op_t	ne2k_nd_rx_rx_rdy;

#endif

