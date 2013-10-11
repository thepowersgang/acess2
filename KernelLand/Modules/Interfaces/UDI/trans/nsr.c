/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * trans/nsr.c
 * - "Network Service Requester" (UDI->IP Translation)
 */
#define DEBUG	1
#include <udi.h>
#include <udi_nic.h>
#include <acess.h>
#include <trans_nsr.h>
#include <IPStack/include/adapters_api.h>
#include <workqueue.h>

#define NUM_RX_BUFFERS	4
#define NUM_TX_DESCS	4

enum {
	ACESSNSR_OPS_CTRL = 1,
	ACESSNSR_OPS_TX,
	ACESSNSR_OPS_RX,
};
enum {
	ACESSNSR_META_NIC = 1
};
enum {
	ACESSNSR_CB_CTRL = 1,
	ACESSNSR_CB_RX,
	ACESSNSR_CB_TX
};

// === TYPES ===
typedef struct acessnsr_txdesc_s
{
	//udi_nic_tx_cb_t	cb;
	tMutex	CompleteMutex;
	 int	BufIdx;
	tIPStackBuffer	*IPBuffer;
} acessnsr_txdesc_t;

typedef struct
{
	udi_init_context_t	init_context;	
	udi_cb_t	*active_cb;

	udi_index_t	init_idx;
	udi_buf_t	*rx_buffers[NUM_RX_BUFFERS];
	udi_nic_rx_cb_t	*rx_cbs[NUM_RX_BUFFERS];

	tWorkqueue	RXQueue;

	tIPStack_AdapterType	AdapterType;
	void	*ipstack_handle;

	tWorkqueue	TXWorkQueue;
	acessnsr_txdesc_t	TXDescs[NUM_TX_DESCS];
	
	udi_channel_t	rx_channel;
	udi_channel_t	tx_channel;
} acessnsr_rdata_t;

// === PROTOTYPES ===
// --- Management metalang
void acessnsr_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level);
void acessnsr_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID);
void acessnsr_final_cleanup_req(udi_mgmt_cb_t *cb);
// --- NSR Control
void acessnsr_ctrl_channel_event_ind(udi_channel_event_cb_t *cb);
void acessnsr_ctrl_ch_ev_ind__rx_channel_spawned(udi_cb_t *gcb, udi_channel_t channel);
void acessnsr_ctrl_ch_ev_ind__tx_channel_spawned(udi_cb_t *gcb, udi_channel_t channel);
void acessnsr_ctrl_bind_ack(udi_nic_bind_cb_t *cb, udi_status_t status);
void acessnsr_ctrl_bind_ack__rx_buf_allocated(udi_cb_t *gcb, udi_buf_t *new_buf);
void acessnsr_ctrl_bind_ack__rx_cb_allocated(udi_cb_t *gcb, udi_cb_t *new_cb);
void acessnsr_ctrl_bind_ack__tx_cb_allocated(udi_cb_t *gcb, udi_cb_t *new_cb);
void acessnsr_ctrl_unbind_ack(udi_nic_cb_t *cb, udi_status_t status);
void acessnsr_ctrl_enable_ack(udi_nic_cb_t *cb, udi_status_t status);
void acessnsr_ctrl_disable_ack(udi_nic_cb_t *cb, udi_status_t status);
void acessnsr_ctrl_ctrl_ack(udi_nic_ctrl_cb_t *cb, udi_status_t status);
void acessnsr_ctrl_status_ind(udi_nic_status_cb_t *cb);
void acessnsr_ctrl_info_ack(udi_nic_info_cb_t *cb);
// --- NSR TX
void acessnsr_tx_channel_event_ind(udi_channel_event_cb_t *cb);
void acessnsr_tx_rdy(udi_nic_tx_cb_t *cb);
void acessnsr_tx_rdy__buffer_cleared(udi_cb_t *gcb, udi_buf_t *buf);
// --- NSR RX
void acessnsr_rx_channel_event_ind(udi_channel_event_cb_t *cb);
void acessnsr_rx_ind(udi_nic_rx_cb_t *cb);
void acessnsr_rx_exp_ind(udi_nic_rx_cb_t *cb);
// --- Acess IPStack
 int	acessnsr_SendPacket(void *Card, tIPStackBuffer *Buffer);
void	acessnsr_SendPacket__buf_write_complete(udi_cb_t *gcb, udi_buf_t *buf);
tIPStackBuffer	*acessnsr_WaitForPacket(void *Card);

// === CODE ===
// --- Management metalang
void acessnsr_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	acessnsr_rdata_t *rdata = UDI_GCB(cb)->context;
	switch(resource_level)
	{
	}

	Workqueue_Init(&rdata->RXQueue, "AcessNSR RX", offsetof(udi_nic_rx_cb_t, chain));
	Workqueue_Init(&rdata->TXWorkQueue, "AcessNSR TX", offsetof(udi_nic_tx_cb_t, chain));

	udi_usage_res(cb);
}
void acessnsr_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID)
{
	UNIMPLEMENTED();
}
void acessnsr_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	UNIMPLEMENTED();
}
// --- NSR Control
void acessnsr_ctrl_channel_event_ind(udi_channel_event_cb_t *cb)
{
	acessnsr_rdata_t *rdata = UDI_GCB(cb)->context;
	switch(cb->event)
	{
	case UDI_CHANNEL_CLOSED:
		break;
	case UDI_CHANNEL_BOUND: {
		rdata->active_cb = UDI_GCB(cb);
		udi_channel_spawn(acessnsr_ctrl_ch_ev_ind__rx_channel_spawned,
			cb->params.parent_bound.bind_cb, UDI_GCB(cb)->channel,
			1, ACESSNSR_OPS_RX, rdata);
		// V V V V
		break; }
	}
}
void acessnsr_ctrl_ch_ev_ind__rx_channel_spawned(udi_cb_t *gcb, udi_channel_t channel)
{
	acessnsr_rdata_t *rdata = gcb->context;
	rdata->rx_channel = channel;
	udi_channel_spawn(acessnsr_ctrl_ch_ev_ind__tx_channel_spawned, gcb, gcb->channel,
		2, ACESSNSR_OPS_TX, rdata);
	// V V V V
}
void acessnsr_ctrl_ch_ev_ind__tx_channel_spawned(udi_cb_t *gcb, udi_channel_t channel)
{
	acessnsr_rdata_t *rdata = gcb->context;
	rdata->tx_channel = channel;
	udi_nic_bind_cb_t *bind_cb = UDI_MCB(gcb, udi_nic_bind_cb_t);
	udi_nd_bind_req(bind_cb, 2, 1);
	// V V V V
}
void acessnsr_ctrl_bind_ack(udi_nic_bind_cb_t *cb, udi_status_t status)
{
	acessnsr_rdata_t *rdata = UDI_GCB(cb)->context;
	// TODO: Parse cb and register with IPStack
	// - Pass on capabilities and media type
	switch(cb->media_type)
	{
	case UDI_NIC_ETHER:	rdata->AdapterType.Type = ADAPTERTYPE_ETHERNET_10M;	break;
	case UDI_NIC_FASTETHER:	rdata->AdapterType.Type = ADAPTERTYPE_ETHERNET_100M;	break;
	case UDI_NIC_GIGETHER:	rdata->AdapterType.Type = ADAPTERTYPE_ETHERNET_1G;	break;
	default:
		udi_channel_event_complete( UDI_MCB(rdata->active_cb,udi_channel_event_cb_t), UDI_OK );
		break;
	}

	if(cb->capabilities & UDI_NIC_CAP_TX_IP_CKSUM)
		rdata->AdapterType.Flags |= ADAPTERFLAG_OFFLOAD_IP4;
	if(cb->capabilities & UDI_NIC_CAP_TX_TCP_CKSUM)
		rdata->AdapterType.Flags |= ADAPTERFLAG_OFFLOAD_TCP;
	if(cb->capabilities & UDI_NIC_CAP_TX_UDP_CKSUM)
		rdata->AdapterType.Flags |= ADAPTERFLAG_OFFLOAD_UDP;
	
	rdata->AdapterType.Name = "udi";
	rdata->AdapterType.SendPacket = acessnsr_SendPacket;
	rdata->AdapterType.WaitForPacket = acessnsr_WaitForPacket;

	rdata->ipstack_handle = IPStack_Adapter_Add(&rdata->AdapterType, rdata, cb->mac_addr);

	// Allocate RX CBs and buffers
	rdata->init_idx = -1;
	acessnsr_ctrl_bind_ack__rx_buf_allocated(rdata->active_cb, NULL);
	// V V V V
}
void acessnsr_ctrl_bind_ack__rx_buf_allocated(udi_cb_t *gcb, udi_buf_t *new_buf)
{
	acessnsr_rdata_t *rdata = gcb->context;
	if( rdata->init_idx != (udi_index_t)-1 )
	{
		rdata->rx_buffers[rdata->init_idx] = new_buf;
	}
	rdata->init_idx ++;
	if( rdata->init_idx < NUM_RX_BUFFERS )
	{
		UDI_BUF_ALLOC(acessnsr_ctrl_bind_ack__rx_buf_allocated, gcb, NULL, 0, NULL);
		// A A A A
		return ;
	}
	
	rdata->init_idx = -1;
	acessnsr_ctrl_bind_ack__rx_cb_allocated(gcb, NULL);
}
void acessnsr_ctrl_bind_ack__rx_cb_allocated(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	acessnsr_rdata_t *rdata = gcb->context;
	if( rdata->init_idx != (udi_index_t)-1 )
	{
		udi_nic_rx_cb_t	*cb = UDI_MCB(new_cb, udi_nic_rx_cb_t);
		rdata->rx_cbs[rdata->init_idx] = cb;
		cb->rx_buf = rdata->rx_buffers[rdata->init_idx];
		udi_nd_rx_rdy(cb);
	}
	rdata->init_idx ++;
	if( rdata->init_idx < NUM_RX_BUFFERS )
	{
		udi_cb_alloc(acessnsr_ctrl_bind_ack__rx_cb_allocated, gcb, ACESSNSR_CB_RX, rdata->rx_channel);
		// A A A A
		return ;
	}
	rdata->init_idx = -1;
	acessnsr_ctrl_bind_ack__tx_cb_allocated(gcb, NULL);
	// V V V V
}
void acessnsr_ctrl_bind_ack__tx_cb_allocated(udi_cb_t *gcb, udi_cb_t *new_cb)
{
	acessnsr_rdata_t *rdata = gcb->context;
	if( rdata->init_idx != (udi_index_t)-1 )
	{
		//udi_assert(new_cb);
		ASSERT(new_cb);
		Workqueue_AddWork( &rdata->TXWorkQueue,  UDI_MCB(new_cb, udi_nic_tx_cb_t) );
	}
	rdata->init_idx ++;
	if( rdata->init_idx < NUM_TX_DESCS )
	{
		udi_cb_alloc(acessnsr_ctrl_bind_ack__tx_cb_allocated, gcb, ACESSNSR_CB_TX, rdata->tx_channel);
		// A A A A
		return ;
	}
	udi_channel_event_complete( UDI_MCB(rdata->active_cb,udi_channel_event_cb_t), UDI_OK );
	// = = = =
}
void acessnsr_ctrl_unbind_ack(udi_nic_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}
void acessnsr_ctrl_enable_ack(udi_nic_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}
void acessnsr_ctrl_disable_ack(udi_nic_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}
void acessnsr_ctrl_ctrl_ack(udi_nic_ctrl_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}
void acessnsr_ctrl_status_ind(udi_nic_status_cb_t *cb)
{
	UNIMPLEMENTED();
}
void acessnsr_ctrl_info_ack(udi_nic_info_cb_t *cb)
{
	UNIMPLEMENTED();
}
// --- NSR TX
void acessnsr_tx_channel_event_ind(udi_channel_event_cb_t *cb)
{
	UNIMPLEMENTED();
}
void acessnsr_tx_rdy(udi_nic_tx_cb_t *cb)
{
	//acessnsr_txdesc_t *tx = UDI_GCB(cb)->context;
	// TODO: Can errors be detected here?
	UDI_BUF_DELETE(acessnsr_tx_rdy__buffer_cleared, UDI_GCB(cb), cb->tx_buf->buf_size, cb->tx_buf, 0);
}
void acessnsr_tx_rdy__buffer_cleared(udi_cb_t *gcb, udi_buf_t *buf)
{
	acessnsr_txdesc_t *tx = gcb->scratch;
	udi_nic_tx_cb_t *cb = UDI_MCB(gcb, udi_nic_tx_cb_t);
	cb->tx_buf = buf;
	Mutex_Release(&tx->CompleteMutex);	// triggers acessnsr_SendPacket
}
// --- NSR RX
void acessnsr_rx_channel_event_ind(udi_channel_event_cb_t *cb)
{
	UNIMPLEMENTED();
}
void acessnsr_rx_ind(udi_nic_rx_cb_t *cb)
{
	acessnsr_rdata_t *rdata = UDI_GCB(cb)->context;
	udi_nic_rx_cb_t *next;
	do {
		next = cb->chain;
		Workqueue_AddWork(&rdata->RXQueue, cb);
	} while( (cb = next) );
}
void acessnsr_rx_exp_ind(udi_nic_rx_cb_t *cb)
{
	acessnsr_rx_ind(cb);
	//UNIMPLEMENTED();
}
// --- Acess IPStack
int acessnsr_SendPacket(void *Card, tIPStackBuffer *Buffer)
{
	acessnsr_rdata_t *rdata = Card;
	udi_nic_tx_cb_t	*cb = Workqueue_GetWork(&rdata->TXWorkQueue);
	ASSERT(cb);
	acessnsr_txdesc_t *tx = UDI_GCB(cb)->scratch;
	ASSERT(tx);
	
	Mutex_Acquire(&tx->CompleteMutex);
	tx->IPBuffer = Buffer;
	tx->BufIdx = -1;
	acessnsr_SendPacket__buf_write_complete(UDI_GCB(cb), NULL);

	// Double lock is resolved once TX is complete
	Mutex_Acquire(&tx->CompleteMutex);
	Mutex_Release(&tx->CompleteMutex);
	// TODO: TX status
	
	Workqueue_AddWork(&rdata->TXWorkQueue, tx);
	return 0;
}
void acessnsr_SendPacket__buf_write_complete(udi_cb_t *gcb, udi_buf_t *buf)
{
	acessnsr_txdesc_t *tx = gcb->scratch;
	udi_nic_tx_cb_t	*cb = UDI_MCB(gcb, udi_nic_tx_cb_t);
	if( tx->BufIdx >= 0 ) {
		cb->tx_buf = buf;
	}
	size_t	buflen;
	const void	*bufptr;
	if( (tx->BufIdx = IPStack_Buffer_GetBuffer(tx->IPBuffer, tx->BufIdx, &buflen, &bufptr )) != -1 )
	{
		udi_buf_write(acessnsr_SendPacket__buf_write_complete, gcb,
			bufptr, buflen, cb->tx_buf, (cb->tx_buf ? cb->tx_buf->buf_size : 0), 0, NULL);
		// A A A A
		return ;
	}
	
	udi_nd_tx_req(cb);
	// continued in acessnsr_tx_rdy
}
void _FreeHeapSubBuf(void *Arg, size_t Pre, size_t Post, const void *DataBuf)
{
	free(Arg);
}
tIPStackBuffer *acessnsr_WaitForPacket(void *Card)
{
	acessnsr_rdata_t *rdata = Card;
	udi_nic_rx_cb_t *cb = Workqueue_GetWork(&rdata->RXQueue);

	tIPStackBuffer	*ret = IPStack_Buffer_CreateBuffer(1);
	void	*data = malloc( cb->rx_buf->buf_size );
	udi_buf_read(cb->rx_buf, 0, cb->rx_buf->buf_size, data);
	IPStack_Buffer_AppendSubBuffer(ret, cb->rx_buf->buf_size, 0, data, _FreeHeapSubBuf, data);

	udi_nd_rx_rdy(cb);
	return ret;
}

// === UDI Bindings ===
udi_mgmt_ops_t	acessnsr_mgmt_ops = {
	acessnsr_usage_ind,
	udi_enumerate_no_children,
	acessnsr_devmgmt_req,
	acessnsr_final_cleanup_req
};
udi_ubit8_t	acessnsr_mgmt_ops_flags[4] = {0,0,0,0};

udi_primary_init_t	acessnsr_pri_init = {
	.mgmt_ops = &acessnsr_mgmt_ops,
	.mgmt_op_flags = acessnsr_mgmt_ops_flags,
	.mgmt_scratch_requirement = 0,
	.enumeration_attr_list_length = 4,
	.rdata_size = sizeof(acessnsr_rdata_t),
	.child_data_size = 0,
	.per_parent_paths = 0
};

udi_nsr_ctrl_ops_t	acessnsr_ctrl_ops = {
	acessnsr_ctrl_channel_event_ind,
	acessnsr_ctrl_bind_ack,
	acessnsr_ctrl_unbind_ack,
	acessnsr_ctrl_enable_ack,
	acessnsr_ctrl_ctrl_ack,
	acessnsr_ctrl_info_ack,
	acessnsr_ctrl_status_ind
};
udi_ubit8_t	acessnsr_ctrl_op_flags[7] = {0};

udi_nsr_tx_ops_t	acessnsr_tx_ops = {
	acessnsr_tx_channel_event_ind,
	acessnsr_tx_rdy
};
udi_ubit8_t	acessnsr_tx_ops_flags[2] = {0};

udi_nsr_rx_ops_t	acessnsr_rx_ops = {
	acessnsr_rx_channel_event_ind,
	acessnsr_rx_ind,
	acessnsr_rx_exp_ind
};
udi_ubit8_t	acessnsr_rx_ops_flags[3] = {0};

udi_ops_init_t	acessnsr_ops_list[] = {
	{
		ACESSNSR_OPS_CTRL, ACESSNSR_META_NIC, UDI_NSR_CTRL_OPS_NUM,
		0, (udi_ops_vector_t*)&acessnsr_ctrl_ops, acessnsr_ctrl_op_flags
	},
	{
		ACESSNSR_OPS_TX, ACESSNSR_META_NIC, UDI_NSR_TX_OPS_NUM,
		0, (udi_ops_vector_t*)&acessnsr_tx_ops, acessnsr_tx_ops_flags
	},
	{
		ACESSNSR_OPS_RX, ACESSNSR_META_NIC, UDI_NSR_RX_OPS_NUM,
		0, (udi_ops_vector_t*)&acessnsr_rx_ops, acessnsr_rx_ops_flags
	},
	{0}
};
udi_cb_init_t	acessnsr_cb_init_list[] = {
	{ACESSNSR_CB_CTRL, ACESSNSR_META_NIC, UDI_NIC_BIND_CB_NUM, 0, 0,NULL},
	{ACESSNSR_CB_RX, ACESSNSR_META_NIC, UDI_NIC_RX_CB_NUM, 0, 0,NULL},
	{ACESSNSR_CB_TX, ACESSNSR_META_NIC, UDI_NIC_TX_CB_NUM, sizeof(acessnsr_txdesc_t), 0,NULL},
	{0}
};
const udi_init_t	acessnsr_init = {
	.primary_init_info = &acessnsr_pri_init,
	.ops_init_list = acessnsr_ops_list,
	.cb_init_list = acessnsr_cb_init_list,
};
const char	acessnsr_udiprops[] = 
	"properties_version 0x101\0"
	"message 1 Acess2 Kernel\0"
	"message 2 John Hodge (acess@mutabah.net)\0"
	"message 3 Acess2 NSR\0"
	"supplier 1\0"
	"contact 2\0"
	"name 3\0"
	"module acess_nsr\0"
	"shortname acessnsr\0"
	"requires udi 0x101\0"
	"requires udi_nic 0x101\0"
	"meta 1 udi_nic\0"
	"message 101 Ethernet Adapter\0"
	"device 101 1 if_media string eth\0"
	"parent_bind_ops 1 0 1 1\0"
	"\0";
size_t	acessnsr_udiprops_size = sizeof(acessnsr_udiprops);

