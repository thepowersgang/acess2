/**
 * \file udi_meta_gio.h
 */
#ifndef _UDI_META_GIO_H_
#define _UDI_META_GIO_H_

typedef const struct udi_gio_provider_ops_s	udi_gio_provider_ops_t;
typedef const struct udi_gio_client_ops_s	udi_gio_client_ops_t;
typedef struct udi_gio_bind_cb_s	udi_gio_bind_cb_t;
typedef struct udi_gio_xfer_cb_s	udi_gio_xfer_cb_t;
typedef struct udi_gio_rw_params_s	udi_gio_rw_params_t;
typedef struct udi_gio_event_cb_s	udi_gio_event_cb_t;

typedef void	udi_gio_bind_req_op_t(udi_gio_bind_cb_t *cb);
typedef void	udi_gio_unbind_req_op_t(udi_gio_bind_cb_t *cb);
typedef void	udi_gio_xfer_req_op_t(udi_gio_xfer_cb_t *cb);
typedef void	udi_gio_event_res_op_t(udi_gio_event_cb_t *cb);

typedef void	udi_gio_bind_ack_op_t(
	udi_gio_bind_cb_t *cb,
	udi_ubit32_t	device_size_lo,
	udi_ubit32_t	device_size_hi,
	udi_status_t	status
	);
typedef void	udi_gio_unbind_ack_op_t(udi_gio_bind_cb_t *cb);
typedef void	udi_gio_xfer_ack_op_t(udi_gio_xfer_cb_t *cb);
typedef void	udi_gio_xfer_nak_op_t(udi_gio_xfer_cb_t *cb, udi_status_t status);
typedef void	udi_gio_event_ind_op_t(udi_gio_event_cb_t *cb);

typedef udi_ubit8_t	udi_gio_op_t;
/* Limit values for udi_gio_op_t */
#define UDI_GIO_OP_CUSTOM                 16
#define UDI_GIO_OP_MAX                    64
/* Direction flag values for op */
#define UDI_GIO_DIR_READ                  (1U<<6)
#define UDI_GIO_DIR_WRITE                 (1U<<7)
/* Standard Operation Codes */
#define UDI_GIO_OP_READ       UDI_GIO_DIR_READ
#define UDI_GIO_OP_WRITE      UDI_GIO_DIR_WRITE



struct udi_gio_provider_ops_s
{
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_gio_bind_req_op_t	*gio_bind_req_op;
	udi_gio_unbind_req_op_t	*gio_unbind_req_op;
	udi_gio_xfer_req_op_t	*gio_xfer_req_op;
	udi_gio_event_res_op_t	*gio_event_res_op;
};
/* Ops Vector Number */
#define UDI_GIO_PROVIDER_OPS_NUM          1

struct udi_gio_client_ops_s
{
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_gio_bind_ack_op_t	*gio_bind_ack_op;
	udi_gio_unbind_ack_op_t	*gio_unbind_ack_op;
	udi_gio_xfer_ack_op_t	*gio_xfer_ack_op;
	udi_gio_xfer_nak_op_t	*gio_xfer_nak_op;
	udi_gio_event_ind_op_t	*gio_event_ind_op;
};
/* Ops Vector Number */
#define UDI_GIO_CLIENT_OPS_NUM            2

struct udi_gio_bind_cb_s
{
	udi_cb_t	gcb;
	udi_xfer_constraints_t	xfer_constraints;
};
/* Control Block Group Number */
#define UDI_GIO_BIND_CB_NUM      1


struct udi_gio_xfer_cb_s
{
	udi_cb_t	gcb;
	udi_gio_op_t	op;
	void	*tr_params;
	udi_buf_t	*data_buf;
};
/* Control Block Group Number */
#define UDI_GIO_XFER_CB_NUM      2

struct udi_gio_rw_params_s
{
	udi_ubit32_t offset_lo;
	udi_ubit32_t offset_hi;
};

struct udi_gio_event_cb_s
{
	udi_cb_t	gcb;
	udi_ubit8_t	event_code;
	void	*event_params;
};
/* Control Block Group Number */
#define UDI_GIO_EVENT_CB_NUM     3


extern udi_gio_bind_req_op_t	udi_gio_bind_req;
extern udi_gio_bind_ack_op_t	udi_gio_bind_ack;

extern udi_gio_unbind_req_op_t	udi_gio_unbind_req;
extern udi_gio_unbind_ack_op_t	udi_gio_unbind_ack;

extern udi_gio_xfer_req_op_t	udi_gio_xfer_req;
extern udi_gio_xfer_ack_op_t	udi_gio_xfer_ack;
extern udi_gio_xfer_nak_op_t	udi_gio_xfer_nak;

extern udi_gio_event_ind_op_t	udi_gio_event_ind;
extern udi_gio_event_ind_op_t	udi_gio_event_res_unused;
extern udi_gio_event_res_op_t	udi_gio_event_res;
extern udi_gio_event_res_op_t	udi_gio_event_res_unused;

#endif
