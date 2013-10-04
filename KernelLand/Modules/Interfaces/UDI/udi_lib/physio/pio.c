/**
 * \file physio/pio.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_physio.h>

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
	char bus_type[16];
	udi_instance_attr_type_t	type;
	type = udi_instance_attr_get_internal(gcb, "bus_type", 0, bus_type, sizeof(bus_type), NULL);
	if(type != UDI_ATTR_STRING) {
		callback(gcb, UDI_NULL_HANDLE);
		return ;
	}
	
	if( strcmp(bus_type, "pci") == 0 ) {
		// Ask PCI binding
	}
	else {
		// Oops, unknown
		callback(gcb, UDI_NULL_HANDLE);
		return ;
	}
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

