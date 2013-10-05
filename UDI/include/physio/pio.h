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
/**
 * \brief Register numbers in pio_op
 */
enum {
	UDI_PIO_R0,
	UDI_PIO_R1,
	UDI_PIO_R2,
	UDI_PIO_R3,
	UDI_PIO_R4,
	UDI_PIO_R5,
	UDI_PIO_R6,
	UDI_PIO_R7,
};
// Addressing modes
#define UDI_PIO_DIRECT	0x00
#define UDI_PIO_SCRATCH	0x08
#define UDI_PIO_BUF	0x10
#define UDI_PIO_MEM	0x18
// Class A opcodes
#define	UDI_PIO_IN	0x00
#define	UDI_PIO_OUT	0x20
#define	UDI_PIO_LOAD	0x40
#define	UDI_PIO_STORE	0x60
// Class B opcodes
#define	UDI_PIO_LOAD_IMM	0x80
#define	UDI_PIO_CSKIP	0x88
#define	UDI_PIO_IN_IND	0x90
#define	UDI_PIO_OUT_IND	0x98
#define	UDI_PIO_SHIFT_LEFT	0xA0
#define	UDI_PIO_SHIFT_RIGHT	0xA8
#define	UDI_PIO_AND	0xB0
#define	UDI_PIO_AND_IMM	0xB8
#define	UDI_PIO_OR	0xC0
#define	UDI_PIO_OR_IMM	0xC8
#define	UDI_PIO_XOR	0xD0
#define	UDI_PIO_ADD	0xD8
#define	UDI_PIO_ADD_IMM	0xE0
#define	UDI_PIO_SUB	0xE8
// Class C opcodes
#define	UDI_PIO_BRANCH	0xF0
#define	UDI_PIO_LABEL	0xF1
#define	UDI_PIO_REP_IN_IND	0xF2
#define	UDI_PIO_REP_OUT_IND	0xF3
#define	UDI_PIO_DELAY	0xF4
#define	UDI_PIO_BARRIER	0xF5
#define	UDI_PIO_SYNC	0xF6
#define	UDI_PIO_SYNC_OUT	0xF7
#define	UDI_PIO_DEBUG	0xF8
#define	UDI_PIO_END	0xFE
#define	UDI_PIO_END_IMM	0xFF
// Values for UDI_PIO_DEBUG's operand
#define	UDI_PIO_TRACE_OPS_NONE	0
#define	UDI_PIO_TRACE_OPS1	1
#define	UDI_PIO_TRACE_OPS2	2
#define	UDI_PIO_TRACE_OPS3	3
#define	UDI_PIO_TRACE_REGS_NONE	(0U<<2)
#define	UDI_PIO_TRACE_REGS1	(1U<<2)
#define	UDI_PIO_TRACE_REGS2	(2U<<2)
#define	UDI_PIO_TRACE_REGS3	(3U<<2)
#define	UDI_PIO_TRACE_DEV_NONE	(0U<<4)
#define	UDI_PIO_TRACE_DEV1	(1U<<4)
#define	UDI_PIO_TRACE_DEV2	(2U<<4)
#define	UDI_PIO_TRACE_DEV3	(3U<<4)
// Values for conditional operations
#define UDI_PIO_Z	0	// reg == 0
#define UDI_PIO_NZ	1	// reg != 0
#define UDI_PIO_NEG	2	// reg < 0 (signed)
#define UDI_PIO_NNEG	3	// reg >= 0 (signed)

#define UDI_PIO_REP_ARGS(mode,mem_reg,mem_stride,pio_reg,pio_stride,cnt_reg) \
	((mode)|(mem_reg)|((mem_stride)<<5)|((pio_reg)<<7)|((pio_stride)<<10)|((cnt_reg)<<13))

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

extern void udi_pio_probe(udi_pio_probe_call_t *callback, udi_cb_t *gcb,
	udi_pio_handle_t pio_handle, void *mem_ptr, udi_ubit32_t pio_offset,
	udi_ubit8_t tran_size, udi_ubit8_t direction);

/**
 * \}
 */

#endif
