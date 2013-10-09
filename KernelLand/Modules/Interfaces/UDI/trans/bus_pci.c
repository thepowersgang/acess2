/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * trans/bus_pci.c
 * - PCI Bus Driver
 */
#define DEBUG	1
#include <udi.h>
#include <udi_physio.h>
#include <udi_pci.h>
#include <acess.h>
#include <drv_pci.h>	// acess
#include <udi_internal.h>
#include <trans_pci.h>

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

#define PCI_OPS_BRIDGE	1
#define PCI_OPS_IRQ	2

#define PCI_MAX_EVENT_CBS	8

// === TYPES ===
typedef struct
{
	udi_init_context_t	init_context;
	
	tPCIDev	cur_iter;
} pci_rdata_t;

typedef struct
{
	udi_child_chan_context_t	child_chan_context;
	
	udi_channel_t	interrupt_channel;
	struct {
		tPAddr	paddr;
		void	*vaddr;
		size_t	length;
	} bars[6];

	 int	interrupt_handle;

	udi_pio_handle_t 	intr_preprocessing;	
	udi_intr_event_cb_t	*event_cbs[PCI_MAX_EVENT_CBS];
	 int	event_cb_wr_ofs;
	 int	event_cb_rd_ofs;
} pci_child_chan_context_t;

// === PROTOTYPES ===
void	pci_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level);
void	pci_enumerate_req(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level);
void	pci_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID);
void	pci_final_cleanup_req(udi_mgmt_cb_t *cb);

void	pci_bridge_ch_event_ind(udi_channel_event_cb_t *cb);
void	pci_unbind_req(udi_bus_bind_cb_t *cb);
void	pci_bind_req_op(udi_bus_bind_cb_t *cb);
void	pci_intr_attach_req(udi_intr_attach_cb_t *cb);
void	pci_intr_attach_req__channel_spawned(udi_cb_t *gcb, udi_channel_t new_channel);
void	pci_intr_detach_req(udi_intr_detach_cb_t *cb);

void	pci_intr_ch_event_ind(udi_channel_event_cb_t *cb);
void	pci_intr_event_rdy(udi_intr_event_cb_t *cb);
void	pci_intr_handler(int irq, void *void_context);
void	pci_intr_handle__trans_done(udi_cb_t *gcb, udi_buf_t *new_buf, udi_status_t status, udi_ubit16_t result);

// - Hook to physio (UDI doesn't define these)
 int	pci_pio_get_regset(udi_cb_t *gcb, udi_ubit32_t regset_idx, void **baseptr, size_t *lenptr);

// === CODE ===
void pci_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	pci_rdata_t	*rdata = UDI_GCB(cb)->context;
	
	switch(cb->meta_idx)
	{
	case 1:	// mgmt
		break;
	}

	switch(resource_level)
	{
	case UDI_RESOURCES_CRITICAL:
	case UDI_RESOURCES_LOW:
	case UDI_RESOURCES_NORMAL:
	case UDI_RESOURCES_PLENTIFUL:
		break;
	}

	// TODO: Initialise rdata
	rdata->cur_iter = -1;

	udi_usage_res(cb);
}
void pci_enumerate_req(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level)
{
	pci_rdata_t	*rdata = UDI_GCB(cb)->context;
	switch(enumeration_level)
	{
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:
		rdata->cur_iter = -1;
	case UDI_ENUMERATE_NEXT:
		// TODO: Filters
		if( (rdata->cur_iter = PCI_GetDeviceByClass(0,0, rdata->cur_iter)) == -1 )
		{
			udi_enumerate_ack(cb, UDI_ENUMERATE_DONE, 0);
		}
		else
		{
			udi_instance_attr_list_t *attr_list = cb->attr_list;
			Uint16	ven, dev;
			Uint32	class;
			PCI_GetDeviceInfo(rdata->cur_iter, &ven, &dev, &class);
			Uint8	revision;
			PCI_GetDeviceVersion(rdata->cur_iter, &revision);
			Uint16	sven, sdev;
			PCI_GetDeviceSubsys(rdata->cur_iter, &sven, &sdev);

			udi_strcpy(attr_list->attr_name, "identifier");
			attr_list->attr_length = sprintf((char*)attr_list->attr_value,
				"%04x%04x%02x%04x%04x",
				ven, dev, revision, sven, sdev);
			attr_list ++;
			DPT_SET_ATTR_STRING(attr_list, "bus_type", "pci", 3);
			attr_list ++;
			DPT_SET_ATTR32(attr_list, "pci_vendor_id", ven);
			attr_list ++;
			DPT_SET_ATTR32(attr_list, "pci_device_id", dev);
			attr_list ++;

			cb->attr_valid_length = attr_list - cb->attr_list;
			cb->child_ID = rdata->cur_iter;
			udi_enumerate_ack(cb, UDI_ENUMERATE_OK, 1);
		}
		break;
	}
}
void pci_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID)
{
	UNIMPLEMENTED();
}
void pci_final_cleanup_req(udi_mgmt_cb_t *cb)
{
	UNIMPLEMENTED();
}

void pci_bridge_ch_event_ind(udi_channel_event_cb_t *cb)
{
	UNIMPLEMENTED();
}
void pci_bind_req(udi_bus_bind_cb_t *cb)
{
	// TODO: "Lock" PCI device

	// TODO: DMA constraints
	udi_bus_bind_ack(cb, 0, UDI_DMA_LITTLE_ENDIAN, UDI_OK);
}
void pci_unbind_req(udi_bus_bind_cb_t *cb)
{
	UNIMPLEMENTED();
}
void pci_intr_attach_req(udi_intr_attach_cb_t *cb)
{
	pci_child_chan_context_t *context = UDI_GCB(cb)->context;

	ASSERT(cb->interrupt_idx == 0);	

	context->intr_preprocessing = cb->preprocessing_handle;
	// Check if interrupt is already bound
	if( !UDI_HANDLE_IS_NULL(context->interrupt_channel, udi_channel_t) )
	{
		udi_intr_attach_ack(cb, UDI_OK);
		return ;
	}
	// Create a channel
	udi_channel_spawn(pci_intr_attach_req__channel_spawned, UDI_GCB(cb),
		cb->gcb.channel, cb->interrupt_idx, PCI_OPS_IRQ, context);
}
void pci_intr_attach_req__channel_spawned(udi_cb_t *gcb, udi_channel_t new_channel)
{
	udi_intr_attach_cb_t *cb = UDI_MCB(gcb, udi_intr_attach_cb_t);
	pci_child_chan_context_t *context = UDI_GCB(cb)->context;

	if( UDI_HANDLE_IS_NULL(new_channel, udi_channel_t) )
	{
		// oops
		return ;
	}	

	context->interrupt_channel = new_channel;
	
	context->interrupt_handle = IRQ_AddHandler(
		PCI_GetIRQ(context->child_chan_context.child_ID),
		pci_intr_handler, new_channel);

	udi_intr_attach_ack(cb, UDI_OK);
}
void pci_intr_detach_req(udi_intr_detach_cb_t *cb)
{
	UNIMPLEMENTED();
}

void pci_intr_ch_event_ind(udi_channel_event_cb_t *cb)
{
	UNIMPLEMENTED();
}
void pci_intr_event_rdy(udi_intr_event_cb_t *cb)
{
	pci_child_chan_context_t	*context = UDI_GCB(cb)->context;
	if( context->event_cbs[context->event_cb_wr_ofs] )
	{
		// oops, overrun.
		return ;
	}
	context->event_cbs[context->event_cb_wr_ofs++] = cb;
	if( context->event_cb_wr_ofs == PCI_MAX_EVENT_CBS )
		context->event_cb_wr_ofs = 0;
}

void pci_intr_handler(int irq, void *void_context)
{
	pci_child_chan_context_t *context = void_context;

	if( context->event_cb_rd_ofs == context->event_cb_wr_ofs ) {
		// Dropped
		return ;
	}

	udi_intr_event_cb_t *cb = context->event_cbs[context->event_cb_rd_ofs];
	context->event_cbs[context->event_cb_rd_ofs] = NULL;
	context->event_cb_rd_ofs ++;
	if( context->event_cb_rd_ofs == PCI_MAX_EVENT_CBS )
		context->event_cb_rd_ofs = 0;
	
	if( UDI_HANDLE_IS_NULL(context->intr_preprocessing, udi_pio_handle_t) )
	{
		udi_intr_event_ind(cb, 0);
	}
	else
	{
		// Processing
		// - no event info, so mem_ptr=NULL
		udi_pio_trans(pci_intr_handle__trans_done, UDI_GCB(cb),
			context->intr_preprocessing, 1, cb->event_buf, NULL);
	}
}

void pci_intr_handle__trans_done(udi_cb_t *gcb, udi_buf_t *new_buf, udi_status_t status, udi_ubit16_t result)
{
	udi_intr_event_cb_t *cb = UDI_MCB(gcb, udi_intr_event_cb_t);
	
	cb->intr_result = result;
	
	udi_intr_event_ind(cb, UDI_INTR_PREPROCESSED);	
}

// - physio hooks
udi_status_t pci_pio_do_io(uint32_t child_ID, udi_ubit32_t regset_idx, udi_ubit32_t ofs, udi_ubit8_t len,
	void *data, bool isOutput)
{
//	LOG("child_ID=%i, regset_idx=%i,ofs=0x%x,len=%i,data=%p,isOutput=%b",
//		child_ID, regset_idx, ofs, len, data, isOutput);
	tPCIDev	pciid = child_ID;
	// TODO: Cache child mappings	

	switch(regset_idx)
	{
	case UDI_PCI_CONFIG_SPACE:
		// TODO:
		return UDI_STAT_NOT_SUPPORTED;
	case UDI_PCI_BAR_0 ... UDI_PCI_BAR_5: {
		Uint32 bar = PCI_GetBAR(pciid, regset_idx);
		if(bar & 1)
		{
			// IO BAR
			bar &= ~3;
			#define _IO(fc, type) do {\
				if( isOutput ) { \
					LOG("out"#fc"(0x%x, 0x%x)",bar+ofs,*(type*)data);\
					out##fc(bar+ofs, *(type*)data); \
				} \
				else { \
					*(type*)data = in##fc(bar+ofs); \
					LOG("in"#fc"(0x%x) = 0x%x",bar+ofs,*(type*)data);\
				}\
				} while(0)
			switch(len)
			{
			case UDI_PIO_1BYTE:	_IO(b, udi_ubit8_t);	return UDI_OK;
			case UDI_PIO_2BYTE:	_IO(w, udi_ubit16_t);	return UDI_OK;
			case UDI_PIO_4BYTE:	_IO(d, udi_ubit32_t);	return UDI_OK;
			//case UDI_PIO_8BYTE:	_IO(q, uint64_t);	return UDI_OK;
			default:
				return UDI_STAT_NOT_SUPPORTED;
			}
			#undef _IO
		}
		else
		{
			// Memory BAR
			//Uint64 longbar = PCI_GetValidBAR(pciid, regset_idx, PCI_BARTYPE_MEM);
			return UDI_STAT_NOT_SUPPORTED;
		}
		break; }
	default:
		return UDI_STAT_NOT_UNDERSTOOD;
	}
}

// === UDI Functions ===
udi_mgmt_ops_t	pci_mgmt_ops = {
	pci_usage_ind,
	pci_enumerate_req,
	pci_devmgmt_req,
	pci_final_cleanup_req
};
udi_ubit8_t	pci_mgmt_op_flags[4] = {0,0,0,0};
udi_bus_bridge_ops_t	pci_bridge_ops = {
	pci_bridge_ch_event_ind,
	pci_bind_req,
	pci_unbind_req,
	pci_intr_attach_req,
	pci_intr_detach_req
};
udi_ubit8_t	pci_bridge_op_flags[5] = {0,0,0,0,0};
udi_intr_dispatcher_ops_t	pci_irq_ops = {
	pci_intr_ch_event_ind,
	pci_intr_event_rdy
};
udi_ubit8_t	pci_irq_ops_flags[2] = {0,0};
udi_primary_init_t	pci_pri_init = {
	.mgmt_ops = &pci_mgmt_ops,
	.mgmt_op_flags = pci_mgmt_op_flags,
	.mgmt_scratch_requirement = 0,
	.enumeration_attr_list_length = 4,
	.rdata_size = sizeof(pci_rdata_t),
	.child_data_size = 0,
	.per_parent_paths = 0
};
udi_ops_init_t	pci_ops_list[] = {
	{
		PCI_OPS_BRIDGE, 1, UDI_BUS_BRIDGE_OPS_NUM,
		sizeof(pci_child_chan_context_t),
		(udi_ops_vector_t*)&pci_bridge_ops,
		pci_bridge_op_flags
	},
	{
		PCI_OPS_IRQ, 1, UDI_BUS_INTR_DISPATCH_OPS_NUM,
		0,
		(udi_ops_vector_t*)&pci_irq_ops,
		pci_irq_ops_flags
	},
	{0}
};
udi_init_t	pci_init = {
	.primary_init_info = &pci_pri_init,
	.ops_init_list = pci_ops_list
};
const char	pci_udiprops[] =
	"properties_version 0x101\0"
	"message 1 Acess2 Kernel\0"
	"message 2 John Hodge (acess@mutabah.net)\0"
	"message 3 Acess2 PCI Bus\0"
	"supplier 1\0"
	"contact 2\0"
	"name 3\0"
	"module acess_pci\0"
	"shortname acesspci\0"
	"requires udi 0x101\0"
	"provides udi_bridge 0x101\0"
	"meta 1 udi_bridge\0"
	"enumerates 4 0 100 1 bus_name string pci\0"
	"region 0\0"
	"child_bind_ops 1 0 1\0"
	"";
size_t	pci_udiprops_size = sizeof(pci_udiprops);
