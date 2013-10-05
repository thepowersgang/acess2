/**
 * \file udi_scsi.h
 * \brief UDI SCSI Bindings
 */
#ifndef _UDI_SCSI_H_
#define _UDI_SCSI_H_

typedef struct {
	udi_cb_t	gcb;
	udi_ubit16_t	events;
} udi_scsi_bind_cb_t;

#define UDI_SCSI_BIND_CB_NUM	1

/* SCSI Events */
#define UDI_SCSI_EVENT_AEN	(1U<<0)
#define UDI_SCSI_EVENT_TGT_RESET	(1U<<1)
#define UDI_SCSI_EVENT_BUS_RESET	(1U<<2)
#define UDI_SCSI_EVENT_UNSOLICITED_RESELECT	(1U<<3)

typedef struct {
	udi_cb_t	gcb;
	udi_buf_t	*data_buf;
	udi_ubit32_t	timeout;
	udi_ubit16_t	flags;
	udi_ubit8_t	attribute;
	udi_ubit8_t	cdb_len;
	udi_ubit8_t	*cdb_ptr;
} udi_scsi_io_cb_t;
/* Control Block Group Number */
#define UDI_SCSI_IO_CB_NUM 2
/* I/O Request Flags */
#define UDI_SCSI_DATA_IN (1U<<0)
#define UDI_SCSI_DATA_OUT (1U<<1)
#define UDI_SCSI_NO_DISCONNECT (1U<<2)
/* SCSI Task Attributes */
#define UDI_SCSI_SIMPLE_TASK 1
#define UDI_SCSI_ORDERED_TASK 2
#define UDI_SCSI_HEAD_OF_Q_TASK 3
#define UDI_SCSI_ACA_TASK 4
#define UDI_SCSI_UNTAGGED_TASK 5

typedef struct {
	udi_status_t	req_status;
	udi_ubit8_t	scsi_status;
	udi_ubit8_t	sense_status;
} udi_scsi_status_t;

typedef struct {
	udi_cb_t	gcb;
	udi_ubit8_t	ctrl_func;
	udi_ubit16_t	queue_depth;
} udi_scsi_ctl_cb_t;
/* Values for ctrl_func */
#define UDI_SCSI_CTL_ABORT_TASK_SET	1
#define UDI_SCSI_CTL_CLEAR_TASK_SET	2
#define UDI_SCSI_CTL_LUN_RESET  	3
#define UDI_SCSI_CTL_TGT_RESET  	4
#define UDI_SCSI_CTL_BUS_RESET  	5
#define UDI_SCSI_CTL_CLEAR_ACA  	6
#define UDI_SCSI_CTL_SET_QUEUE_DEPTH	7
/* Control Block Group Number */
#define UDI_SCSI_CTL_CB_NUM	3

typedef struct {
	udi_cb_t	gcb;
	udi_ubit8_t	event;
	udi_buf_t	*aen_data_buf;
} udi_scsi_event_cb_t;
/* Control Block Group Number */
#define UDI_SCSI_EVENT_CB_NUM 4

typedef void udi_scsi_bind_ack_op_t(udi_scsi_bind_cb_t *cb, udi_ubit32_t hd_timeout_increase, udi_status_t status);
typedef void udi_scsi_unbind_ack_op_t(udi_scsi_bind_cb_t *cb);
typedef void udi_scsi_io_ack_op_t(udi_scsi_io_cb_t *cb);
typedef void udi_scsi_io_nak_op_t(udi_scsi_io_cb_t *cb);
typedef void udi_scsi_ctl_ack_op_t(udi_scsi_ctl_cb_t *cb, udi_status_t status);
typedef void udi_scsi_event_ind_op_t(udi_scsi_event_cb_t *cb);

typedef void udi_scsi_bind_req_op_t(udi_scsi_bind_cb_t *cb,
	udi_ubit16_t bind_flags, udi_ubit16_t queue_depth,
	udi_ubit16_t max_sense_len, udi_ubit16_t aen_buf_size);
typedef void udi_scsi_unbind_req_op_t(udi_scsi_bind_cb_t *cb);
typedef void udi_scsi_io_req_op_t(udi_scsi_io_cb_t *cb);
typedef void udi_scsi_ctl_req_op_t(udi_scsi_ctl_cb_t *cb);
typedef void udi_scsi_event_res_op_t(udi_scsi_event_cb_t *cb);

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_scsi_bind_ack_op_t	*bind_ack_op;
	udi_scsi_unbind_ack_op_t	*unbind_ack_op;
	udi_scsi_io_ack_op_t	*io_ack_op;
	udi_scsi_io_nak_op_t	*io_nak_op;
	udi_scsi_ctl_ack_op_t	*ctl_ack_op;
	udi_scsi_event_ind_op_t	*event_ind_op;
} udi_scsi_pd_ops_t;

#define UDI_SCSI_PD_OPS_NUM	1

typedef const struct {
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_scsi_bind_req_op_t	*bind_req_op;
	udi_scsi_unbind_req_op_t	*unbind_req_op;
	udi_scsi_io_req_op_t	*io_req_op;
	udi_scsi_ctl_req_op_t	*ctl_req_op;
	udi_scsi_event_res_op_t	*event_res_op;
} udi_scsi_hd_ops_t;

#define UDI_SCSI_HD_OPS_NUM	2

/* Bind Flags */
#define UDI_SCSI_BIND_EXCLUSIVE (1U<<0)
#define UDI_SCSI_TEMP_BIND_EXCLUSIVE (1U<<1)

extern void udi_scsi_bind_req(udi_scsi_bind_cb_t *cb,
	udi_ubit16_t bind_flags, udi_ubit16_t queue_depth,
	udi_ubit16_t max_sense_len, udi_ubit16_t aen_buf_size);
extern void udi_scsi_bind_ack(udi_scsi_bind_cb_t *cb, udi_ubit32_t hd_timeout_increase, udi_status_t status);
extern void udi_scsi_unbind_req(udi_scsi_bind_cb_t *cb);
extern void udi_scsi_unbind_ack(udi_scsi_bind_cb_t *cb);

extern void udi_scsi_io_req(udi_scsi_io_cb_t *cb);
extern void udi_scsi_io_ack(udi_scsi_io_cb_t *cb);
extern void udi_scsi_io_nak(udi_scsi_io_cb_t *cb, udi_scsi_status_t status, udi_buf_t *sense_buf);
extern void udi_scsi_ctl_req(udi_scsi_ctl_cb_t *cb);
extern void udi_scsi_ctl_ack(udi_scsi_ctl_cb_t *cb, udi_status_t status);
extern void udi_scsi_event_ind(udi_scsi_event_cb_t *cb);
extern udi_scsi_event_ind_op_t	udi_scsi_event_ind_unused;
extern void udi_scsi_event_res(udi_scsi_event_cb_t *cb);
extern void udi_scsi_inquiry_to_string(const udi_ubit8_t *inquiry_data, udi_size_t inquiry_len, char *str);


#endif

