/**
 * \file udi_mem.h
 */
#ifndef _UDI_MEM_H_
#define _UDI_MEM_H_

/**
 * \brief Callback type for ::udi_mem_alloc
 */
typedef void udi_mem_alloc_call_t(udi_cb_t *gcb, void *new_mem);

/**
 * \brief Allocate memory
 */
extern void udi_mem_alloc(
	udi_mem_alloc_call_t *callback,
	udi_cb_t	*gcb,
	udi_size_t	size,
	udi_ubit8_t	flags
	);

/**
 * \brief Values for ::udi_mem_alloc \a flags
 * \{
 */
#define UDI_MEM_NOZERO               (1U<<0)	//!< No need to zero the memory
#define UDI_MEM_MOVABLE              (1U<<1)	//!< Globally accessable memory?
/**
 * \}
 */

/**
 * \brief Free allocated memory
 */
extern void udi_mem_free(void *target_mem);


#endif
