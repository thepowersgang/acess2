/**
 * \file udi_lib/physio/pio.c
 * \author John Hodge (thePowersGang)
 */
#define DEBUG	1
#include <udi.h>
#include <udi_physio.h>
#include <acess.h>
#include <udi_internal.h>

typedef void	_udi_pio_do_io_op_t(uint32_t child_ID, udi_ubit32_t regset_idx, udi_ubit32_t ofs, size_t len,
	void *data, bool isOutput);

// === STRUCTURES ===
struct udi_pio_handle_s
{
	tUDI_DriverInstance	*Inst;
	udi_index_t	RegionIdx;
	
	_udi_pio_do_io_op_t	*IOFunc;
	udi_ubit32_t	Offset;
	udi_ubit32_t	Length;
	
	size_t	nTransOps;
	udi_pio_trans_t	*TransOps;
	
	udi_ubit16_t	PIOAttributes;
	udi_ubit32_t	Pace;
	udi_index_t	Domain;

	// TODO: Cached labels
};

// === IMPORTS ===
extern _udi_pio_do_io_op_t	pci_pio_do_io;

// === EXPORTS ===
EXPORT(udi_pio_map);
EXPORT(udi_pio_unmap);
EXPORT(udi_pio_atmic_sizes);
EXPORT(udi_pio_abort_sequence);
EXPORT(udi_pio_trans);
EXPORT(udi_pio_probe);

// === CODE ===
void udi_pio_map(udi_pio_map_call_t *callback, udi_cb_t *gcb,
	udi_ubit32_t regset_idx, udi_ubit32_t base_offset, udi_ubit32_t length,
	udi_pio_trans_t *trans_list, udi_ubit16_t list_length,
	udi_ubit16_t pio_attributes, udi_ubit32_t pace, udi_index_t serialization_domain)
{
	LOG("gcb=%p,regset_idx=%i,base_offset=0x%x,length=0x%x,trans_list=%p,list_length=%i,...",
		gcb, regset_idx, base_offset, length, trans_list, list_length);
	char bus_type[16];
	udi_instance_attr_type_t	type;
	type = udi_instance_attr_get_internal(gcb, "bus_type", 0, bus_type, sizeof(bus_type), NULL);
	if(type != UDI_ATTR_STRING) {
		Log_Warning("UDI", "No/invalid bus_type attribute");
		callback(gcb, UDI_NULL_PIO_HANDLE);
		return ;
	}


	_udi_pio_do_io_op_t	*io_op;
	if( strcmp(bus_type, "pci") == 0 ) {
		// Ask PCI binding
		io_op = pci_pio_do_io;
	}
	else {
		// Oops, unknown
		callback(gcb, UDI_NULL_PIO_HANDLE);
		return ;
	}

	udi_pio_handle_t ret = malloc( sizeof(struct udi_pio_handle_s) );
	ret->Inst = UDI_int_ChannelGetInstance(gcb, false, &ret->RegionIdx);
	ret->IOFunc = io_op;
	ret->Offset = base_offset;
	ret->Length = length;
	ret->TransOps = trans_list;
	// TODO: Pre-scan to get labels
	ret->nTransOps = list_length;
	ret->PIOAttributes = pio_attributes;
	ret->Pace = pace;
	// TODO: Validate serialization_domain
	ret->Domain = serialization_domain;
	
	callback(gcb, ret);
}

void udi_pio_unmap(udi_pio_handle_t pio_handle)
{
	UNIMPLEMENTED();
}

udi_ubit32_t udi_pio_atmic_sizes(udi_pio_handle_t pio_handle)
{
	UNIMPLEMENTED();
	return 0;
}

void udi_pio_abort_sequence(udi_pio_handle_t pio_handle, udi_size_t scratch_requirement)
{
	UNIMPLEMENTED();
}

void udi_pio_trans(udi_pio_trans_call_t *callback, udi_cb_t *gcb,
	udi_pio_handle_t pio_handle, udi_index_t start_label,
	udi_buf_t *buf, void *mem_ptr)
{
	UNIMPLEMENTED();
}

void udi_pio_probe(udi_pio_probe_call_t *callback, udi_cb_t *gcb,
	udi_pio_handle_t pio_handle, void *mem_ptr, udi_ubit32_t pio_offset,
	udi_ubit8_t tran_size, udi_ubit8_t direction)
{
	UNIMPLEMENTED();
}

