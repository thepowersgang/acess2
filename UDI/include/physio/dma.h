/*
 */
#ifndef _UDI_PHYSIO_DMA_H_
#define _UDI_PHYSIO_DMA_H_

//typedef struct udi_dma_handle_s	*udi_dma_handle_t;
//#define UDI_NULL_DMA_HANDLE	NULL

extern void udi_dma_limits(udi_dma_limits_t *dma_limits);

typedef void udi_dma_prepare_call_t(udi_cb_t *gcb, udi_dma_handle_t new_dma_handle);
typedef void udi_dma_buf_map_call_t(udi_cb_t *gcb, udi_scgth_t *scgth, udi_boolean_t complete, udi_status_t status);
typedef void udi_dma_mem_alloc_call_t(udi_cb_t *gcb, udi_dma_handle_t new_dma_handle, void *mem_ptr, udi_size_t actual_gap, udi_boolean_t single_element, udi_scgth_t *scgth, udi_boolean_t must_swap);
typedef void udi_dma_sync_call_t(udi_cb_t *gcb);
typedef void udi_dma_scgth_sync_call_t(udi_cb_t *gcb);
typedef void udi_dma_mem_to_buf_call_t(udi_cb_t *gcb, udi_buf_t *new_dst_buf);

/**
 * \name Values for flags (udi_dma_prepare, udi_dma_buf_map)
 * \{
 */
#define UDI_DMA_OUT	(1U<<2)
#define UDI_DMA_IN	(1U<<3)
#define UDI_DMA_REWIND	(1U<<4)
#define UDI_DMA_BIG_ENDIAN	(1U<<5)
#define UDI_DMA_LITTLE_ENDIAN	(1U<<6)
#define UDI_DMA_NEVERSWAP	(1U<<7)
/**
 * \}
 */

extern void udi_dma_prepare(udi_dma_prepare_call_t *callback, udi_cb_t *gcb,
	udi_dma_constraints_t constraints, udi_ubit8_t flags);

extern void udi_dma_buf_map(udi_dma_buf_map_call_t *callback, udi_cb_t *gcb, udi_dma_handle_t dma_handle,
	udi_buf_t *buf, udi_size_t offset, udi_size_t len, udi_ubit8_t flags);

extern udi_buf_t *udi_dma_buf_unmap(udi_dma_handle_t dma_handle, udi_size_t new_buf_size);

extern void udi_dma_mem_alloc(udi_dma_mem_alloc_call_t *callback, udi_cb_t *gcb,
	udi_dma_constraints_t constraints, udi_ubit8_t flags,
	udi_ubit16_t nelements, udi_size_t element_size, udi_size_t max_gap);

extern void udi_dma_sync(udi_dma_sync_call_t *callback, udi_cb_t *gcb, udi_dma_handle_t dma_handle,
	udi_size_t offset, udi_size_t len, udi_ubit8_t flags);

extern void udi_dma_scgth_sync(udi_dma_scgth_sync_call_t *callback, udi_cb_t *gcb, udi_dma_handle_t dma_handle);

extern void udi_dma_mem_barrier(udi_dma_handle_t dma_handle);

extern void udi_dma_free(udi_dma_handle_t dma_handle);

extern void udi_dma_mem_to_buf(udi_dma_mem_to_buf_call_t *callback, udi_cb_t *gcb, udi_dma_handle_t dma_handle,
	udi_size_t src_off, udi_size_t src_len, udi_buf_t *dst_buf);

#endif

