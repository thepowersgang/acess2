/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * trans/bus_pci.c
 * - PCI Bus Driver
 */
#include <udi.h>
#include <udi_physio.h>
#include <udi_pci.h>
#include <acess.h>
#include <drv_pci.h>	// acess

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


// === TYPES ===
typedef struct
{
	udi_init_context_t	init_context;
	
	tPCIDev	cur_iter;
} pci_rdata_t;

// === PROTOTYPES ===
void	pci_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level);
void	pci_enumerate_req(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level);
void	pci_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID);
void	pci_final_cleanup_req(udi_mgmt_cb_t *cb);

void	pci_bridge_ch_event_ind(udi_channel_event_cb_t *cb);
void	pci_unbind_req(udi_bus_bind_cb_t *cb);
void	pci_bind_req_op(udi_bus_bind_cb_t *cb);
void	pci_intr_attach_req(udi_intr_attach_cb_t *cb);
void	pci_intr_detach_req(udi_intr_detach_cb_t *cb);


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
			attr_list->attr_length = snprintf(attr_list->attr_value,
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
	UNIMPLEMENTED();
}
void pci_intr_detach_req(udi_intr_detach_cb_t *cb)
{
	UNIMPLEMENTED();
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
		1, 1, UDI_BUS_BRIDGE_OPS_NUM,
		0,
		(udi_ops_vector_t*)&pci_bridge_ops,
		pci_bridge_op_flags
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
