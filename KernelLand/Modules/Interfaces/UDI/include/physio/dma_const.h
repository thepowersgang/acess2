/*
 * 
 */
#ifndef _UDI_DMA_CONST_H_
#define _UDI_DMA_CONST_H_

typedef void udi_dma_constraints_attr_set_call_t(udi_cb_t *gcb, udi_dma_constraints_t new_constraints, udi_status_t status);

/**
 * \name Flags for udi_dma_constraints_attr_set
 * \{
 */
#define UDI_DMA_CONSTRAINTS_COPY	(1U<<0)
/**
 * \}
 */

extern void udi_dma_constraints_attr_set(udi_dma_constraints_attr_set_call_t *callback, udi_cb_t *gcb,
	udi_dma_constraints_t src_constraints,
	const udi_dma_constraints_attr_spec_t *attr_list, udi_ubit16_t list_length,
	udi_ubit8_t flags);

extern void udi_dma_constraints_attr_reset(udi_dma_constraints_t constraints, udi_dma_constraints_attr_t attr_type);

extern void udi_dma_constraints_free(udi_dma_constraints_t constraints);

#endif
