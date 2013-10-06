/**
 * \file physio/meta_intr.h
 */
#ifndef _PHYSIO_META_INTR_H_
#define _PHYSIO_META_INTR_H_

#include <udi.h>
#include <udi_physio.h>
#include "pio.h"

typedef struct udi_intr_attach_cb_s	udi_intr_attach_cb_t;
typedef void	udi_intr_attach_req_op_t(udi_intr_attach_cb_t *intr_attach_cb);
typedef void	udi_intr_attach_ack_op_t(
	udi_intr_attach_cb_t *intr_attach_cb,
	udi_status_t status
	);
typedef struct udi_intr_detach_cb_s	udi_intr_detach_cb_t;
typedef void	udi_intr_detach_req_op_t(udi_intr_detach_cb_t *intr_detach_cb);
typedef void	udi_intr_detach_ack_op_t(udi_intr_detach_cb_t *intr_detach_cb);
typedef const struct udi_intr_handler_ops_s	udi_intr_handler_ops_t;
typedef const struct udi_intr_dispatcher_ops_s	udi_intr_dispatcher_ops_t;
typedef struct udi_intr_event_cb_s	udi_intr_event_cb_t;
typedef void	udi_intr_event_ind_op_t(udi_intr_event_cb_t *intr_event_cb, udi_ubit8_t flags);
typedef void	udi_intr_event_rdy_op_t(udi_intr_event_cb_t *intr_event_cb);


struct udi_intr_attach_cb_s
{
	udi_cb_t	gcb;
	udi_index_t	interrupt_idx;
	udi_ubit8_t	min_event_pend;
	udi_pio_handle_t	preprocessing_handle;
};
/* Bridge Attach Control Block Group Number */
#define UDI_BUS_INTR_ATTACH_CB_NUM        2

struct udi_intr_detach_cb_s
{
	udi_cb_t	gcb;
	udi_index_t	interrupt_idx;
};
/* Bridge Detach Control Block Group Number */
#define UDI_BUS_INTR_DETACH_CB_NUM       3

struct udi_intr_handler_ops_s
{
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_intr_event_ind_op_t	*intr_event_ind_op;
};
/* Interrupt Handler Ops Vector Number */
#define UDI_BUS_INTR_HANDLER_OPS_NUM      3

struct udi_intr_dispatcher_ops_s
{
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_intr_event_rdy_op_t	*intr_event_rdy_op;
};
/* Interrupt Dispatcher Ops Vector Number */
#define UDI_BUS_INTR_DISPATCH_OPS_NUM     4

struct udi_intr_event_cb_s
{
	udi_cb_t	gcb;
	udi_buf_t	*event_buf;
	udi_ubit16_t	intr_result;
};
/* Flag values for interrupt handling */
#define UDI_INTR_UNCLAIMED               (1U<<0)
#define UDI_INTR_NO_EVENT                (1U<<1)
/* Bus Interrupt Event Control Block Group Number */
#define UDI_BUS_INTR_EVENT_CB_NUM        4



extern udi_intr_attach_req_op_t udi_intr_attach_req;
extern udi_intr_attach_ack_op_t	udi_intr_attach_ack;
extern udi_intr_attach_ack_op_t	udi_intr_attach_ack_unused;

extern void udi_intr_detach_req(udi_intr_detach_cb_t *intr_detach_cb);
extern void udi_intr_detach_ack(udi_intr_detach_cb_t *intr_detach_cb);
extern udi_intr_detach_ack_op_t	udi_intr_detach_ack_unused;


extern void udi_intr_event_ind(udi_intr_event_cb_t *intr_event_cb, udi_ubit8_t flags);
/**
 * \brief Values for ::udi_intr_event_ind \a flags
 * \{
 */
#define UDI_INTR_MASKING_NOT_REQUIRED    (1U<<0)
#define UDI_INTR_OVERRUN_OCCURRED        (1U<<1)
#define UDI_INTR_PREPROCESSED            (1U<<2)
/**
 * \}
 */

extern void udi_intr_event_rdy(udi_intr_event_cb_t *intr_event_cb);



#endif
