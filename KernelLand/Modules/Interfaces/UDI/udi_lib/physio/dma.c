/**
 * \file physio/dma.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_physio.h>

// === EXPORTS ===
EXPORT(udi_dma_prepare);
EXPORT(udi_dma_buf_map);
EXPORT(udi_dma_buf_unmap);
EXPORT(udi_dma_mem_alloc);
EXPORT(udi_dma_sync);
EXPORT(udi_dma_scgth_sync);
EXPORT(udi_dma_mem_barrier);
EXPORT(udi_dma_free);
EXPORT(udi_dma_mem_to_buf);

// === CODE ===
void udi_dma_prepare(udi_dma_prepare_call_t *callback, udi_cb_t *gcb,
	udi_dma_constraints_t constraints, udi_ubit8_t flags)
{
	UNIMPLEMENTED();
}

void udi_dma_buf_map(udi_dma_buf_map_call_t *callback, udi_cb_t *gcb, udi_dma_handle_t dma_handle,
	udi_buf_t *buf, udi_size_t offset, udi_size_t len, udi_ubit8_t flags)
{
	UNIMPLEMENTED();
}

udi_buf_t *udi_dma_buf_unmap(udi_dma_handle_t dma_handle, udi_size_t new_buf_size)
{
	UNIMPLEMENTED();
	return NULL;
}

void udi_dma_mem_alloc(udi_dma_mem_alloc_call_t *callback, udi_cb_t *gcb,
	udi_dma_constraints_t constraints, udi_ubit8_t flags,
	udi_ubit16_t nelements, udi_size_t element_size, udi_size_t max_gap)
{
	UNIMPLEMENTED();
}

void udi_dma_sync(udi_dma_sync_call_t *callback, udi_cb_t *gcb, udi_dma_handle_t dma_handle,
	udi_size_t offset, udi_size_t len, udi_ubit8_t flags)
{
	UNIMPLEMENTED();
}

void udi_dma_scgth_sync(udi_dma_scgth_sync_call_t *callback, udi_cb_t *gcb, udi_dma_handle_t dma_handle)
{
	UNIMPLEMENTED();
}

void udi_dma_mem_barrier(udi_dma_handle_t dma_handle)
{
	UNIMPLEMENTED();
}

void udi_dma_free(udi_dma_handle_t dma_handle)
{
	UNIMPLEMENTED();
}

void udi_dma_mem_to_buf(udi_dma_mem_to_buf_call_t *callback, udi_cb_t *gcb, udi_dma_handle_t dma_handle,
	udi_size_t src_off, udi_size_t src_len, udi_buf_t *dst_buf)
{
	UNIMPLEMENTED();
}
