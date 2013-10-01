/**
 * \file udi_physio.h
 */
#ifndef _UDI_PHYSIO_H_
#define _UDI_PHYSIO_H_

#include <udi.h>

// === TYPEDEFS ===
// DMA Core
typedef _udi_handle_t	udi_dma_handle_t;
#define	UDI_NULL_DMA_HANDLE	_NULL_HANDLE
typedef uint64_t	udi_busaddr64_t;	//!< \note Opaque
typedef struct udi_scgth_element_32_s	udi_scgth_element_32_t;
typedef struct udi_scgth_element_64_s	udi_scgth_element_64_t;
typedef struct udi_scgth_s	udi_scgth_t;
typedef _udi_handle_t	udi_dma_constraints_t;
#define UDI_NULL_DMA_CONSTRAINTS	_NULL_HANDLE
/**
 * \name DMA constraints attributes
 * \{
 */
typedef udi_ubit8_t udi_dma_constraints_attr_t;
/* DMA Convenience Attribute Codes */
#define UDI_DMA_ADDRESSABLE_BITS          100
#define UDI_DMA_ALIGNMENT_BITS            101
/* DMA Constraints on the Entire Transfer */
#define UDI_DMA_DATA_ADDRESSABLE_BITS     110
#define UDI_DMA_NO_PARTIAL                111
/* DMA Constraints on the Scatter/Gather  List */
#define UDI_DMA_SCGTH_MAX_ELEMENTS        120
#define UDI_DMA_SCGTH_FORMAT              121
#define UDI_DMA_SCGTH_ENDIANNESS          122
#define UDI_DMA_SCGTH_ADDRESSABLE_BITS    123
#define UDI_DMA_SCGTH_MAX_SEGMENTS        124
/* DMA Constraints on Scatter/Gather Segments */
#define UDI_DMA_SCGTH_ALIGNMENT_BITS      130
#define UDI_DMA_SCGTH_MAX_EL_PER_SEG      131
#define UDI_DMA_SCGTH_PREFIX_BYTES        132
/* DMA Constraints on Scatter/Gather Elements */
#define UDI_DMA_ELEMENT_ALIGNMENT_BITS    140
#define UDI_DMA_ELEMENT_LENGTH_BITS       141
#define UDI_DMA_ELEMENT_GRANULARITY_BITS 142
/* DMA Constraints for Special Addressing */
#define UDI_DMA_ADDR_FIXED_BITS           150
#define UDI_DMA_ADDR_FIXED_TYPE           151
#define UDI_DMA_ADDR_FIXED_VALUE_LO       152
#define UDI_DMA_ADDR_FIXED_VALUE_HI       153
/* DMA Constraints on DMA Access Behavior */
#define UDI_DMA_SEQUENTIAL                160
#define UDI_DMA_SLOP_IN_BITS              161
#define UDI_DMA_SLOP_OUT_BITS             162
#define UDI_DMA_SLOP_OUT_EXTRA            163
#define UDI_DMA_SLOP_BARRIER_BITS         164
/* Values for UDI_DMA_SCGTH_ENDIANNESS */
#define UDI_DMA_LITTLE_ENDIAN             (1U<<6)
#define UDI_DMA_BIG_ENDIAN                (1U<<5)
/* Values for UDI_DMA_ADDR_FIXED_TYPE */
#define UDI_DMA_FIXED_ELEMENT             1
/**
 * \}
 */
// DMA Constraints Management
typedef struct udi_dma_constraints_attr_spec_s	udi_dma_constraints_attr_spec_t;
typedef void udi_dma_constraints_attr_set_call_t(
	udi_cb_t *gcb, udi_dma_constraints_t new_constraints, udi_status_t status
	);
typedef	struct udi_dma_limits_s	udi_dma_limits_t;


// === STRUCTURES ===
// --- DMA Constraints Management ---
struct udi_dma_constraints_attr_spec_s
{
     udi_dma_constraints_attr_t	attr_type;
     udi_ubit32_t	attr_value;
};
// --- DMA Core ---
struct udi_dma_limits_s
{
     udi_size_t max_legal_contig_alloc;
     udi_size_t max_safe_contig_alloc;
     udi_size_t cache_line_size;
};
struct udi_scgth_element_32_s
{
     udi_ubit32_t	block_busaddr;
     udi_ubit32_t	block_length;
};
struct udi_scgth_element_64_s
{
     udi_busaddr64_t	block_busaddr;
     udi_ubit32_t	block_length;
     udi_ubit32_t	el_reserved;
};
/* Extension Flag */
#define UDI_SCGTH_EXT                    0x80000000
struct udi_scgth_s
{
     udi_ubit16_t	scgth_num_elements;
     udi_ubit8_t	scgth_format;
     udi_boolean_t	scgth_must_swap;
     union {
          udi_scgth_element_32_t	*el32p;
          udi_scgth_element_64_t	*el64p;
     }	scgth_elements;
     union {
          udi_scgth_element_32_t	el32;
          udi_scgth_element_64_t	el64;
     }	scgth_first_segment;
};
/* Values for scgth_format */
#define UDI_SCGTH_32                     (1U<<0)
#define UDI_SCGTH_64                     (1U<<1)
#define UDI_SCGTH_DMA_MAPPED             (1U<<6)
#define UDI_SCGTH_DRIVER_MAPPED          (1U<<7)



// === FUNCTIONS ===
#include <physio/dma_const.h>
#include <physio/dma.h>
#include <physio/meta_intr.h>
#include <physio/meta_bus.h>
#include <physio/pio.h>

#endif
