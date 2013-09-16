/**
 * \file physio/pio.h
 */
#ifndef _PHYSIO_PIO_H_
#define _PHYSIO_PIO_H_

#include <udi.h>
#include <udi_physio.h>
typedef _udi_handle_t	udi_pio_handle_t;
/* Null handle value for udi_pio_handle_t */
#define UDI_NULL_PIO_HANDLE	_NULL_HANDLE

typedef void udi_pio_map_call_t(udi_cb_t *gcb, udi_pio_handle_t new_pio_handle);

typedef const struct {
	udi_ubit8_t	pio_op;
	udi_ubit8_t	tran_size;
	udi_ubit16_t	operand;
} udi_pio_trans_t;

/**
 * \brief Values for tran_size
 */
enum {
	UDI_PIO_1BYTE,
	UDI_PIO_2BYTE,
	UDI_PIO_4BYTE,
	UDI_PIO_8BYTE,
	UDI_PIO_16BYTE,
	UDI_PIO_32BYTE,
};

//! \brief PIO Handle Layout Element Type Code
#define UDI_DL_PIO_HANDLE_T	200

/**
 * \name PIO Handle Allocation and Initialisation
 * \{
 */

/**
 * \name Values for pio_attributes of udi_pio_map
 * \{
 */
#define UDI_PIO_STRICTORDER	(1U<<0)
#define UDI_PIO_UNORDERED_OK	(1U<<1)
#define UDI_PIO_MERGING_OK	(1U<<2)
#define UDI_PIO_LOADCACHING_OK	(1U<<3)
#define UDI_PIO_STORECACHING_OK	(1U<<4)
#define UDI_PIO_BIG_ENDIAN	(1U<<5)
#define UDI_PIO_LITTLE_ENDIAN	(1U<<6)
#define UDI_PIO_NEVERSWAP	(1U<<7)
#define UDI_PIO_UNALIGNED	(1U<<8)
/**
 * \}
 */

extern void udi_pio_map(udi_pio_map_call_t *callback, udi_cb_t *gcb,
	udi_ubit32_t regset_idx, udi_ubit32_t base_offset, udi_ubit32_t length,
	udi_pio_trans_t *trans_list, udi_ubit16_t list_length,
	udi_ubit16_t pio_attributes, udi_ubit32_t pace, udi_index_t serialization_domain);

extern void udi_pio_unmap(udi_pio_handle_t pio_handle);

extern udi_ubit32_t udi_pio_atmic_sizes(udi_pio_handle_t pio_handle);

extern void udi_pio_abort_sequence(udi_pio_handle_t pio_handle, udi_size_t scratch_requirement);

/**
 * \}
 */

/**
 * \name PIO Access Service Calls
 * \{
 */
typedef void udi_pio_trans_call_t(udi_cb_t *gcb, udi_buf_t *new_buf, udi_status_t status, udi_ubit16_t result);

extern void udi_pio_trans(udi_pio_trans_call_t *callback, udi_cb_t *gcb,
	udi_pio_handle_t pio_handle, udi_index_t start_label,
	udi_buf_t *buf, void *mem_ptr);

typedef void udi_pio_probe_call_t(udi_cb_t *gcb, udi_status_t status);

/**
 * \name Values for direction
 * \{
 */
#define UDI_PIO_IN	0x00
#define UDI_PIO_OUT	0x20
/**
 * \}
 */

extern void udi_pio_probe(udi_pio_probe_call_t *callback, udi_cb_t *gcb,
	udi_pio_handle_t pio_handle, void *mem_ptr, udi_ubit32_t pio_offset,
	udi_ubit8_t tran_size, udi_ubit8_t direction);

/**
 * \}
 */

#endif
