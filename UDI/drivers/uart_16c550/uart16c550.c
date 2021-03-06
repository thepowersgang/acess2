/*
 * UDI Ne2000 NIC Driver
 * By John Hodge (thePowersGang)
 *
 * uart16c550.c
 * - UDI initialisation 
 */
#define UDI_VERSION	0x101
#define UDI_PCI_VERSION	0x101
#define UDI_PHYSIO_VERSION	0x101
#include <udi.h>
#include <udi_physio.h>
#include <udi_pci.h>
#include "uart16c550_common.h"

#include "uart16c550_pio.h"

#if 0
# define DEBUG_OUT(fmt, v...)	udi_debug_printf("%s: "fmt"\n", __func__ ,## v) 
#else
# define DEBUG_OUT(...)	do{}while(0)
#endif

#define __EXPJOIN(a,b)	a##b
#define _EXPJOIN(a,b)	__EXPJOIN(a,b)
#define _EXPLODE(params...)	params
#define _ADDGCB(params...)	(udi_cb_t *gcb, params)
#define CONTIN(suffix, call, args, params)	extern void _EXPJOIN(suffix##__,__LINE__) _ADDGCB params;\
	call( _EXPJOIN(suffix##__,__LINE__), gcb, _EXPLODE args); } \
	void _EXPJOIN(suffix##__,__LINE__) _ADDGCB params { \
	rdata_t	*rdata = gcb->context;

// --- Management Metalang
void uart_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	rdata_t	*rdata = UDI_GCB(cb)->context;
	//udi_trace_write(rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY, 0, );

	// TODO: Set up region data	

	udi_usage_res(cb);
}

void uart_enumerate_req(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level)
{
	rdata_t	*rdata = UDI_GCB(cb)->context;
	udi_instance_attr_list_t *attr_list = cb->attr_list;
	
	switch(enumeration_level)
	{
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:
		DPT_SET_ATTR_STRING(attr_list, "gio_type", "uart", 4);
		attr_list ++;
		cb->attr_valid_length = attr_list - cb->attr_list;
		udi_enumerate_ack(cb, UDI_ENUMERATE_OK, UART_OPS_GIO);
		break;
	case UDI_ENUMERATE_NEXT:
		udi_enumerate_ack(cb, UDI_ENUMERATE_DONE, 0);
		break;
	}
}
void uart_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID)
{
}
void uart_final_cleanup_req(udi_mgmt_cb_t *cb)
{
}
// ---
void uart_bus_dev_channel_event_ind(udi_channel_event_cb_t *cb);
void uart_bus_dev_bus_bind_ack(udi_bus_bind_cb_t *cb, udi_dma_constraints_t dma_constraints, udi_ubit8_t perferred_endianness, udi_status_t status);
void uart_bus_dev_bind__pio_map(udi_cb_t *cb, udi_pio_handle_t new_pio_handle);
void uart_bus_dev_bind__intr_chanel(udi_cb_t *gcb, udi_channel_t new_channel);
void uart_bus_dev_bus_unbind_ack(udi_bus_bind_cb_t *cb);

void uart_bus_dev_channel_event_ind(udi_channel_event_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	switch(cb->event)
	{
	case UDI_CHANNEL_CLOSED:
		break;
	case UDI_CHANNEL_BOUND: {
		rdata->active_cb = gcb;
		udi_bus_bind_cb_t *bus_bind_cb = UDI_MCB(cb->params.parent_bound.bind_cb, udi_bus_bind_cb_t);
		udi_bus_bind_req( bus_bind_cb );
		// continue at uart_bus_dev_bus_bind_ack
		return; }
	}
}
void uart_bus_dev_bus_bind_ack(udi_bus_bind_cb_t *cb,
	udi_dma_constraints_t dma_constraints, udi_ubit8_t perferred_endianness, udi_status_t status)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	// Set up PIO handles
	rdata->init.pio_index = -1;
	uart_bus_dev_bind__pio_map(gcb, UDI_NULL_PIO_HANDLE);
	// V V V V
}
void uart_bus_dev_bind__pio_map(udi_cb_t *gcb, udi_pio_handle_t new_pio_handle)
{
	rdata_t	*rdata = gcb->context;
	if( rdata->init.pio_index != (udi_index_t)-1 )
	{
		rdata->pio_handles[rdata->init.pio_index] = new_pio_handle;
	}
	
	rdata->init.pio_index ++;
	if( rdata->init.pio_index < N_PIO )
	{
		udi_pio_map(uart_bus_dev_bind__pio_map, gcb,
			UDI_PCI_BAR_0, 0, 0x8,	// TODO: Use system bus index and offset
			uart_pio_ops[rdata->init.pio_index].trans_list,
			uart_pio_ops[rdata->init.pio_index].list_length,
			UDI_PIO_LITTLE_ENDIAN, 0, 0
			);
		return ;
	}

	// Spawn the interrupt channel
	CONTIN( uart_bus_dev_bind, udi_channel_spawn, (gcb->channel, 0, UART_OPS_IRQ, rdata),
		(udi_channel_t new_channel))
	
	rdata->interrupt_channel = new_channel;

	// Allocate an RX buffer
	CONTIN( uart_bus_dev_bind, udi_buf_write, (NULL, 0, NULL, 0, 0, UDI_NULL_BUF_PATH),
		(udi_buf_t *new_buf));	
	if( !new_buf )
	{
		// Oh...
		udi_channel_event_complete( UDI_MCB(rdata->active_cb, udi_channel_event_cb_t),
			UDI_STAT_RESOURCE_UNAVAIL );
		return ;
	}
	rdata->rx_buffer = new_buf;

	// Create interrupt CB
	CONTIN( uart_bus_dev_bind, udi_cb_alloc, (UART_CB_INTR, gcb->channel),
		(udi_cb_t *new_cb))
	if( !new_cb )
	{
		// Oh...
		udi_channel_event_complete( UDI_MCB(rdata->active_cb, udi_channel_event_cb_t),
			UDI_STAT_RESOURCE_UNAVAIL );
		return ;
	}
	udi_intr_attach_cb_t	*intr_cb = UDI_MCB(new_cb, udi_intr_attach_cb_t);
	intr_cb->interrupt_idx = 0;
	intr_cb->min_event_pend = 1;
	intr_cb->preprocessing_handle = rdata->pio_handles[PIO_INTR];
	
	// Attach interrupt
	udi_intr_attach_req(intr_cb);
	// continued in uart_bus_dev_intr_attach_ack
}
void uart_bus_dev_bus_unbind_ack(udi_bus_bind_cb_t *cb)
{
}
void uart_bus_dev_intr_attach_ack(udi_intr_attach_cb_t *intr_attach_cb, udi_status_t status)
{
	udi_cb_t	*gcb = UDI_GCB(intr_attach_cb);
	rdata_t	*rdata = gcb->context;

	// Allocate interrupt cbs
	CONTIN(uart_bus_dev_intr_attach_ack, udi_cb_alloc_batch,
		(UART_CB_INTR_EVENT, NUM_INTR_CBS, TRUE, INTR_CB_BUF_SIZE, UDI_NULL_BUF_PATH),
		(udi_cb_t *first_new_cb))

	while( first_new_cb )
	{
		udi_intr_event_cb_t *cb = UDI_MCB(first_new_cb, udi_intr_event_cb_t);
		first_new_cb = first_new_cb->initiator_context;
		// - Set channel (udi_cb_alloc_batch sets the channel to gcb->channel)
		cb->gcb.channel = rdata->interrupt_channel;
		udi_intr_event_rdy(cb);
	}

	udi_channel_event_cb_t	*channel_cb = UDI_MCB(rdata->active_cb, udi_channel_event_cb_t);
	
	udi_channel_event_complete(channel_cb, UDI_OK);
	// = = = = =
}
void uart_bus_dev_intr_detach_ack(udi_intr_detach_cb_t *intr_detach_cb)
{
}
// ---
void uart_bus_irq_channel_event_ind(udi_channel_event_cb_t *cb)
{
}
void uart_bus_irq_intr_event_ind(udi_intr_event_cb_t *cb, udi_ubit8_t flags)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	DEBUG_OUT("flags=%x, intr_result=%x", flags, cb->intr_result);
	if( cb->intr_result == 0 )
	{
		// An IRQ from something other than RX
		udi_intr_event_rdy(cb);
		return ;
	}

	if( rdata->rx_buffer && rdata->rx_buffer->buf_size + cb->intr_result > MAX_RX_BUFFER_SIZE )
	{
		// Drop, buffer is full
		DEBUG_OUT("dropping %i bytes, full rx buffer", cb->intr_result);
		udi_intr_event_rdy(cb);
		return ;
	}

	// Copy cb->intr_result bytes into the RX buffer
	CONTIN(uart_bus_irq_intr_event_ind, udi_buf_copy,
		(cb->event_buf, 0, cb->intr_result,
			rdata->rx_buffer, rdata->rx_buffer->buf_size, 0,
			UDI_NULL_BUF_PATH),
		(udi_buf_t *new_buf));
	
	udi_intr_event_cb_t	*cb = UDI_MCB(gcb, udi_intr_event_cb_t);
	
	rdata->rx_buffer = new_buf;

	DEBUG_OUT("copied %i bytes", cb->intr_result);

	// Emit a signal to GIO client
	if( rdata->event_cb && !rdata->event_cb_used )
	{
		rdata->event_pending = FALSE;
		rdata->event_cb_used = TRUE;
		udi_gio_event_ind(rdata->event_cb);
	}
	else
	{
		DEBUG_OUT("event_cb=%p, event_cb_used=%i - Queueing",
			rdata->event_cb, rdata->event_cb_used);
		rdata->event_pending = TRUE;
	}

	udi_intr_event_rdy(cb);
}
// ---
void uart_gio_prov_channel_event_ind(udi_channel_event_cb_t *cb);
void uart_gio_prov_bind_req(udi_gio_bind_cb_t *cb);
void uart_gio_prov_xfer_req(udi_gio_xfer_cb_t *cb);
void uart_gio_read_complete_short(udi_cb_t *gcb, udi_buf_t *new_buf);
void uart_gio_read_complete(udi_cb_t *gcb, udi_buf_t *new_buf);
void uart_gio_write_complete(udi_cb_t *gcb, udi_buf_t *new_buf, udi_status_t status, udi_ubit16_t res);
void uart_gio_prov_event_res(udi_gio_event_cb_t *cb);

void uart_gio_prov_channel_event_ind(udi_channel_event_cb_t *cb)
{
}
void uart_gio_prov_bind_req(udi_gio_bind_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	// TODO: Should this device allow multiple children?
	if( rdata->event_cb ) {
		DEBUG_OUT("only one client allowed");
		udi_gio_bind_ack(cb, 0, 0, UDI_STAT_CANNOT_BIND_EXCL);
		return;
	}
	
	// Allocate the event CB
	CONTIN( uart_gio_prov_bind_req, udi_cb_alloc, (UART_CB_GIO_EVENT, gcb->channel),
		(udi_cb_t *new_cb));
	udi_gio_bind_cb_t	*cb = UDI_MCB(gcb, udi_gio_bind_cb_t);
	
	rdata->event_cb = UDI_MCB(new_cb, udi_gio_event_cb_t);
	rdata->event_cb_used = FALSE;
	
	udi_gio_bind_ack(cb, 0, 0, UDI_OK);
}
void uart_gio_prov_unbind_req(udi_gio_bind_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	rdata->event_cb = NULL;
	rdata->event_cb_used = FALSE;
	udi_gio_unbind_ack(cb);
}
void uart_gio_prov_xfer_req(udi_gio_xfer_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	switch(cb->op)
	{
	case UDI_GIO_OP_READ: {
		udi_buf_copy_call_t	*callback;
		udi_size_t	len;
		// Read from local FIFO
		if( rdata->rx_buffer->buf_size < cb->data_buf->buf_size ) {
			len = rdata->rx_buffer->buf_size;
			callback = uart_gio_read_complete_short;
		}
		else {
			len = cb->data_buf->buf_size;
			callback = uart_gio_read_complete;
		}
		DEBUG_OUT("Read %i/%i bytes", len, rdata->rx_buffer->buf_size);
		// Replace entire destination buffer with `len` bytes from source
		udi_buf_copy(callback, gcb,
			rdata->rx_buffer, 0, len,
			cb->data_buf, 0, cb->data_buf->buf_size,
			UDI_NULL_BUF_PATH);
		break; }
	case UDI_GIO_OP_WRITE:
		// Send to device
		udi_pio_trans(uart_gio_write_complete, gcb,
			rdata->pio_handles[PIO_TX], 0, cb->data_buf, &cb->data_buf->buf_size);
		break;
	default:
		udi_gio_xfer_nak(cb, UDI_STAT_NOT_SUPPORTED);
		break;
	}
}
void uart_gio_read_complete_short(udi_cb_t *gcb, udi_buf_t *new_buf)
{
	rdata_t	*rdata = gcb->context;
	udi_gio_xfer_cb_t	*cb = UDI_MCB(gcb, udi_gio_xfer_cb_t);
	cb->data_buf = new_buf;
	
	// Delete read bytes from the RX buffer
	CONTIN(uart_gio_read_complete, udi_buf_write,
		(NULL, 0, rdata->rx_buffer, 0, cb->data_buf->buf_size, UDI_NULL_BUF_PATH),
		(udi_buf_t *new_buf))
	udi_gio_xfer_cb_t	*cb = UDI_MCB(gcb, udi_gio_xfer_cb_t);
	rdata->rx_buffer = new_buf;
	
	// Return underrun
	udi_gio_xfer_nak(cb, UDI_STAT_DATA_UNDERRUN);
}
void uart_gio_read_complete(udi_cb_t *gcb, udi_buf_t *new_buf)
{
	rdata_t	*rdata = gcb->context;
	udi_gio_xfer_cb_t	*cb = UDI_MCB(gcb, udi_gio_xfer_cb_t);
	cb->data_buf = new_buf;
	// Delete read bytes from the RX buffer
	CONTIN(uart_gio_read_complete, udi_buf_write,
		(NULL, 0, rdata->rx_buffer, 0, cb->data_buf->buf_size, UDI_NULL_BUF_PATH),
		(udi_buf_t *new_buf))
	udi_gio_xfer_cb_t	*cb = UDI_MCB(gcb, udi_gio_xfer_cb_t);
	rdata->rx_buffer = new_buf;
	
	// Return completed
	udi_gio_xfer_ack(cb);
}
void uart_gio_write_complete(udi_cb_t *gcb, udi_buf_t *new_buf, udi_status_t status, udi_ubit16_t res)
{
	udi_gio_xfer_cb_t	*cb = UDI_MCB(gcb, udi_gio_xfer_cb_t);
	cb->data_buf = new_buf;
	udi_gio_xfer_ack(cb);
}
void uart_gio_prov_event_res(udi_gio_event_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	// If there was an event during dispatch, re-send
	if( rdata->event_pending )
	{
		udi_gio_event_ind(cb);
		rdata->event_pending = FALSE;
	}
	// otherwise, clear 'Event CB used' flag
	else
	{
		rdata->event_cb_used = FALSE;
	}
}

// ====================================================================
// - Management ops
udi_mgmt_ops_t	uart_mgmt_ops = {
	uart_usage_ind,
	uart_enumerate_req,
	uart_devmgmt_req,
	uart_final_cleanup_req
};
udi_ubit8_t	uart_mgmt_op_flags[4] = {0,0,0,0};
// - Bus Ops
udi_bus_device_ops_t	uart_bus_dev_ops = {
	uart_bus_dev_channel_event_ind,
	uart_bus_dev_bus_bind_ack,
	uart_bus_dev_bus_unbind_ack,
	uart_bus_dev_intr_attach_ack,
	uart_bus_dev_intr_detach_ack
};
udi_ubit8_t	uart_bus_dev_ops_flags[5] = {0};
// - IRQ Ops
udi_intr_handler_ops_t	uart_bus_irq_ops = {
	uart_bus_irq_channel_event_ind,
	uart_bus_irq_intr_event_ind
};
udi_ubit8_t	uart_bus_irq_ops_flags[2] = {0};
// - GIO provider ops
udi_gio_provider_ops_t	uart_gio_prov_ops = {
	uart_gio_prov_channel_event_ind,
	uart_gio_prov_bind_req,
	uart_gio_prov_unbind_req,
	uart_gio_prov_xfer_req,
	uart_gio_prov_event_res
};
udi_ubit8_t	uart_gio_prov_ops_flags[5] = {0};
// --
udi_primary_init_t	uart_pri_init = {
	.mgmt_ops = &uart_mgmt_ops,
	.mgmt_op_flags = uart_mgmt_op_flags,
	.mgmt_scratch_requirement = 0,
	.enumeration_attr_list_length = 4,
	.rdata_size = sizeof(rdata_t),
	.child_data_size = 0,
	.per_parent_paths = 0
};
udi_ops_init_t	uart_ops_list[] = {
	{
		UART_OPS_DEV, UART_META_BUS, UDI_BUS_DEVICE_OPS_NUM,
		0,
		(udi_ops_vector_t*)&uart_bus_dev_ops,
		uart_bus_dev_ops_flags
	},
	{
		UART_OPS_IRQ, UART_META_BUS, UDI_BUS_INTR_HANDLER_OPS_NUM,
		0,
		(udi_ops_vector_t*)&uart_bus_irq_ops,
		uart_bus_irq_ops_flags
	},
	{
		UART_OPS_GIO, UART_META_GIO, UDI_GIO_PROVIDER_OPS_NUM,
		0,
		(udi_ops_vector_t*)&uart_gio_prov_ops,
		uart_gio_prov_ops_flags
	},
	{0}
};
udi_cb_init_t uart_cb_init_list[] = {
	{UART_CB_BUS_BIND,   UART_META_BUS, UDI_BUS_BIND_CB_NUM, 0, 0,NULL},
	{UART_CB_INTR,       UART_META_BUS, UDI_BUS_INTR_ATTACH_CB_NUM, 0, 0,NULL},
	{UART_CB_INTR_EVENT, UART_META_BUS, UDI_BUS_INTR_EVENT_CB_NUM, 0, 0,NULL},
	{UART_CB_GIO_EVENT,  UART_META_GIO, UDI_GIO_EVENT_CB_NUM, 0, 0,NULL},
	{0}
};
const udi_init_t	udi_init_info = {
	.primary_init_info = &uart_pri_init,
	.ops_init_list = uart_ops_list,
	.cb_init_list = uart_cb_init_list,
};
