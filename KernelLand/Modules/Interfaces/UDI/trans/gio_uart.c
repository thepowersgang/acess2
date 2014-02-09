/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * trans/gio_uart.c
 * - GIO UART translation (presents "uart" type GIO drivers as serial ports)
 */
#define DEBUG	0
#include <udi.h>
//#include <udi_gio.h>
#include <acess.h>
#include <drv_pty.h>
#include <trans_uart.h>
#include <workqueue.h>

#define NUM_TX_CBS	2
#define RX_SIZE	32	// Size of a read request

typedef struct {
	udi_init_context_t	init_context;
	udi_cb_t	*active_cb;
	tPTY	*PTYInstance;
	tWorkqueue	CBWorkQueue;
} rdata_t;

enum {
	ACESSUART_CB_BIND = 1,
	ACESSUART_CB_XFER
};
enum {
	ACESSUART_OPS_GIO = 1
};
enum {
	ACESSUART_META_GIO = 1,
};

// === PROTOTYPES ===
void	acessuart_pty_output(void *Handle, size_t Length, const void *Data);

// === CODE ===
void acessuart_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	Workqueue_Init(&rdata->CBWorkQueue, "UDI UART TX", offsetof(udi_gio_xfer_cb_t, gcb.initiator_context));
	udi_usage_res(cb);
}
void acessuart_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID)
{
	UNIMPLEMENTED();
}
void acessuart_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	UNIMPLEMENTED();
}
// ----
void acessuart_channel_event_ind(udi_channel_event_cb_t *cb);
void acessuart_channel_event_ind__tx_cbs_allocated(udi_cb_t *gcb, udi_cb_t *first_cb);
void acessuart_bind_ack(udi_gio_bind_cb_t *cb, udi_ubit32_t device_size_lo, udi_ubit32_t device_size_hi, udi_status_t status);
void acessuart_event_ind__buf_allocated(udi_cb_t *gcb, udi_buf_t *buffer);

void acessuart_channel_event_ind(udi_channel_event_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t *rdata = gcb->context;
	ASSERT(rdata);
	switch(cb->event)
	{
	case UDI_CHANNEL_CLOSED:
		break;
	case UDI_CHANNEL_BOUND: {
		rdata->active_cb = gcb;
		// 
		udi_cb_alloc_batch(acessuart_channel_event_ind__tx_cbs_allocated, cb->params.parent_bound.bind_cb,
			ACESSUART_CB_XFER, NUM_TX_CBS, FALSE, 0, UDI_NULL_BUF_PATH);
		// V V V V
		break; }
	}
}
void acessuart_channel_event_ind__tx_cbs_allocated(udi_cb_t *gcb, udi_cb_t *first_cb)
{
	rdata_t	*rdata = gcb->context;
	udi_gio_bind_cb_t	*cb = UDI_MCB(gcb, udi_gio_bind_cb_t);
	ASSERT(rdata);

	while( first_cb )
	{
		udi_cb_t	*next = first_cb->initiator_context;
		first_cb->initiator_context = NULL;
		Workqueue_AddWork(&rdata->CBWorkQueue, first_cb);
		first_cb = next;
	}

	udi_gio_bind_req(cb);
	// continued in acessuart_bind_ack
}
void acessuart_bind_ack(udi_gio_bind_cb_t *cb, udi_ubit32_t device_size_lo, udi_ubit32_t device_size_hi, udi_status_t status)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t *rdata = gcb->context;
	udi_channel_event_cb_t	*channel_cb = UDI_MCB(rdata->active_cb, udi_channel_event_cb_t);
	
	if( device_size_lo != 0 || device_size_hi != 0 ) {
		// Oops... binding failed. UARTS should not have a size
		udi_channel_event_complete( channel_cb, UDI_STAT_NOT_UNDERSTOOD);
		return ;
	}
	
	// bound, create PTY instance
	rdata->PTYInstance = PTY_Create("serial#", rdata, acessuart_pty_output, NULL, NULL);
	if( !rdata->PTYInstance ) {
		udi_channel_event_complete(channel_cb, UDI_STAT_RESOURCE_UNAVAIL);
		return ;
	}
	
	struct ptymode mode = {
		.OutputMode = PTYBUFFMT_TEXT,
		.InputMode = PTYIMODE_CANON|PTYIMODE_ECHO
	};
	struct ptydims dims = {
		.W = 80, .H = 25,
		.PW = 0, .PH = 0
	};
	PTY_SetAttrib(rdata->PTYInstance, &dims, &mode, 0);
	
	udi_channel_event_complete(channel_cb, UDI_OK);
}
void acessuart_unbind_ack(udi_gio_bind_cb_t *cb)
{
	UNIMPLEMENTED();
}
void acessuart_xfer_ack(udi_gio_xfer_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	if( cb->op == UDI_GIO_OP_WRITE ) {
		// Write, no action required except returning the CB to the pool
		udi_buf_free(cb->data_buf);
		//cb->data_buf = NULL;
		Workqueue_AddWork(&rdata->CBWorkQueue, cb);
	}
	else if( cb->op == UDI_GIO_OP_READ ) {
		// Send data to PTY
		UNIMPLEMENTED();
		// TODO: Since this was a full ACK, request more?
	}
	else {
		// Well, that was unexpected
	}
}
void acessuart_xfer_nak(udi_gio_xfer_cb_t *cb, udi_status_t status)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	if( cb->op == UDI_GIO_OP_READ && status == UDI_STAT_DATA_UNDERRUN )
	{
		udi_size_t	len = cb->data_buf->buf_size;
		if( len == 0 )
		{
			udi_debug_printf("%s: no data read, rx buffer must be empty\n", __func__, len);
			ASSERT(status != UDI_OK);
		}
		else
		{
			char	tmp[len];
			udi_buf_read(cb->data_buf, 0, len, tmp);
			for( int i = 0; i < len; i ++ ) {
				if( tmp[i] == '\r' )
					tmp[i] = '\n';
			}
			
			udi_debug_printf("%s: %i bytes '%.*s'\n", __func__, len, len, tmp);
			PTY_SendInput(rdata->PTYInstance, tmp, len);
		}
		
		udi_buf_free(cb->data_buf);
		
		// if status == OK, then all bytes we requested were read.
		// - In which case, we want to request more
		if( status == UDI_OK ) {
			UDI_BUF_ALLOC(acessuart_event_ind__buf_allocated, gcb, NULL, RX_SIZE, UDI_NULL_BUF_PATH);
			return ;
		}
		
		Workqueue_AddWork(&rdata->CBWorkQueue, cb);
	}
	else {
		UNIMPLEMENTED();
	}
}
void acessuart_event_ind(udi_gio_event_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;

	// ACK event before requesting read
	udi_gio_event_res(cb);
	
	// Begin read request
	udi_gio_xfer_cb_t	*read_cb = Workqueue_GetWork(&rdata->CBWorkQueue);
	UDI_BUF_ALLOC(acessuart_event_ind__buf_allocated, UDI_GCB(read_cb), NULL, RX_SIZE, UDI_NULL_BUF_PATH);
}
void acessuart_event_ind__buf_allocated(udi_cb_t *gcb, udi_buf_t *buffer)
{
	udi_gio_xfer_cb_t	*cb = UDI_MCB(gcb, udi_gio_xfer_cb_t);

	cb->op = UDI_GIO_OP_READ;
	cb->tr_params = NULL;
	cb->data_buf = buffer;
	
	udi_gio_xfer_req(cb);
	// Continued in acessuart_xfer_ack/nak
}


void acessuart_pty_output(void *Handle, size_t Length, const void *Data);
void acessuart_pty_output__buf_allocated(udi_cb_t *gcb, udi_buf_t *buffer);

void acessuart_pty_output(void *Handle, size_t Length, const void *Data)
{
	LOG("Output '%.*s'", Length, Data);
	
	rdata_t *rdata = Handle;
	udi_gio_xfer_cb_t	*cb = Workqueue_GetWork(&rdata->CBWorkQueue);
	udi_cb_t	*gcb = UDI_GCB(cb);
	
	UDI_BUF_ALLOC(acessuart_pty_output__buf_allocated, gcb, Data, Length, UDI_NULL_BUF_PATH);
	// don't bother waiting for tx to complete, workqueue will block when everything is in use
	// - And once buf_alloc returns, the data is copied
}
void acessuart_pty_output__buf_allocated(udi_cb_t *gcb, udi_buf_t *buffer)
{
	//rdata_t	*rdata = gcb->context;
	LOG("buffer = %p\n", buffer);
	udi_gio_xfer_cb_t	*cb = UDI_MCB(gcb, udi_gio_xfer_cb_t);

	cb->op = UDI_GIO_OP_WRITE;
	cb->tr_params = NULL;
	cb->data_buf = buffer;	

	udi_gio_xfer_req(cb);
}


// --------------------------------------------------------------------
udi_mgmt_ops_t	acessuart_mgmt_ops = {
	acessuart_usage_ind,
	udi_enumerate_no_children,
	acessuart_devmgmt_req,
	acessuart_final_cleanup_req
};
udi_ubit8_t	acessuart_mgmt_ops_flags[4] = {0,0,0,0};

udi_primary_init_t	acessuart_pri_init = {
	.mgmt_ops = &acessuart_mgmt_ops,
	.mgmt_op_flags = acessuart_mgmt_ops_flags,
	.mgmt_scratch_requirement = 0,
	.enumeration_attr_list_length = 0,
	.rdata_size = sizeof(rdata_t),
	.child_data_size = 0,
	.per_parent_paths = 0
};

udi_gio_client_ops_t	acessuart_gio_ops = {
	acessuart_channel_event_ind,
	acessuart_bind_ack,
	acessuart_unbind_ack,
	acessuart_xfer_ack,
	acessuart_xfer_nak,
	acessuart_event_ind
};
udi_ubit8_t	acessuart_gio_op_flags[7] = {0};

udi_ops_init_t	acessuart_ops_list[] = {
	{
		ACESSUART_OPS_GIO, ACESSUART_META_GIO, UDI_GIO_CLIENT_OPS_NUM,
		0, (udi_ops_vector_t*)&acessuart_gio_ops, acessuart_gio_op_flags
	},
	{0}
};
udi_cb_init_t	acessuart_cb_init_list[] = {
	{ACESSUART_CB_BIND, ACESSUART_META_GIO, UDI_GIO_BIND_CB_NUM, 0, 0,NULL},
	{ACESSUART_CB_XFER, ACESSUART_META_GIO, UDI_GIO_XFER_CB_NUM, 0, 0,NULL},
	{0}
};
const udi_init_t	acessuart_init = {
	.primary_init_info = &acessuart_pri_init,
	.ops_init_list = acessuart_ops_list,
	.cb_init_list = acessuart_cb_init_list,
};
const char	acessuart_udiprops[] = 
	"properties_version 0x101\0"
	"message 1 Acess2 Kernel\0"
	"message 2 John Hodge (acess@mutabah.net)\0"
	"message 3 Acess2 UART\0"
	"supplier 1\0"
	"contact 2\0"
	"name 3\0"
	"module acess_uart\0"
	"shortname acessuart\0"
	"requires udi 0x101\0"
	"requires udi_gio 0x101\0"
	"meta 1 udi_gio\0"
	"message 101 UART\0"
	"device 101 1 gio_type string uart\0"
	"parent_bind_ops 1 0 1 1\0"
	"\0";
size_t	acessuart_udiprops_size = sizeof(acessuart_udiprops);
