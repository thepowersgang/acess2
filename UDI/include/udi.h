/**
 * \file udi.h
 */
#ifndef _UDI_H_
#define _UDI_H_

#ifndef UDI_VERSION
# error "Please define UDI_VERSION before including"
#endif
#if UDI_VERSION < 0x100
# error "Requesting an unsupported UDI version (pre 1.0)"
#endif
#if UDI_VERSION > 0x101
# error "Requesting an unsupported UDI version (post 1.01)"
#endif

#include <stdint.h>
#include <stdarg.h>

typedef int8_t	udi_sbit8_t;	/* signed 8-bit: -2^7..2^7-1 */
typedef int16_t	udi_sbit16_t;	/* signed 16-bit: -2^15..2^15-1 */
typedef int32_t	udi_sbit32_t;	/* signed 32-bit: -2^31..2^31-1 */
typedef uint8_t 	udi_ubit8_t;	/* unsigned 8-bit: 0..28-1 */
typedef uint16_t	udi_ubit16_t;	/* unsigned 16-bit: 0..216-1 */
typedef uint32_t	udi_ubit32_t;	/* unsigned 32-bit: 0..232-1 */

#ifndef NULL
# define NULL	((void*)0)
#endif

#if UDI_ABI_is_ia32
#include "udi/arch/ia32.h"
#elif UDI_ABI_is_amd64
#include "udi/arch/amd64.h"
#else
#error "Unknown UDI ABI"
#endif

/**
 * \name Values and Flags for udi_status_t
 * \{
 */
#define UDI_STATUS_CODE_MASK		0x0000FFFF
#define UDI_STAT_META_SPECIFIC		0x00008000
#define UDI_SPECIFIC_STATUS_MASK	0x00007FFF
#define UDI_CORRELATE_OFFSET		16
#define UDI_CORRELATE_MASK			0xFFFF0000
/* Common Status Values */
#define UDI_OK						0
#define UDI_STAT_NOT_SUPPORTED		1
#define UDI_STAT_NOT_UNDERSTOOD		2
#define UDI_STAT_INVALID_STATE		3
#define UDI_STAT_MISTAKEN_IDENTITY	4
#define UDI_STAT_ABORTED			5
#define UDI_STAT_TIMEOUT			6
#define UDI_STAT_BUSY				7
#define UDI_STAT_RESOURCE_UNAVAIL	8
#define UDI_STAT_HW_PROBLEM			9
#define UDI_STAT_NOT_RESPONDING		10
#define UDI_STAT_DATA_UNDERRUN		11
#define UDI_STAT_DATA_OVERRUN		12
#define UDI_STAT_DATA_ERROR			13
#define UDI_STAT_PARENT_DRV_ERROR	14
#define UDI_STAT_CANNOT_BIND		15
#define UDI_STAT_CANNOT_BIND_EXCL	16
#define UDI_STAT_TOO_MANY_PARENTS	17
#define UDI_STAT_BAD_PARENT_TYPE	18
#define UDI_STAT_TERMINATED			19
#define UDI_STAT_ATTR_MISMATCH		20
/**
 * \}
 */

/**
 * \name Data Layout Specifiers
 * \{
 */
typedef const udi_ubit8_t	udi_layout_t;
/* Specific-Length Layout Type Codes */
#define UDI_DL_UBIT8_T                   1
#define UDI_DL_SBIT8_T                   2
#define UDI_DL_UBIT16_T                  3
#define UDI_DL_SBIT16_T                  4
#define UDI_DL_UBIT32_T                  5
#define UDI_DL_SBIT32_T                  6
#define UDI_DL_BOOLEAN_T                 7
#define UDI_DL_STATUS_T                  8
/* Abstract Element Layout Type Codes */
#define UDI_DL_INDEX_T                   20
/* Opaque Handle Element Layout Type Codes */
#define UDI_DL_CHANNEL_T                 30
#define UDI_DL_ORIGIN_T                  32
/* Indirect Element Layout Type Codes */
#define UDI_DL_BUF                       40
#define UDI_DL_CB                        41
#define UDI_DL_INLINE_UNTYPED            42
#define UDI_DL_INLINE_DRIVER_TYPED       43
#define UDI_DL_MOVABLE_UNTYPED           44
/* Nested Element Layout Type Codes */
#define UDI_DL_INLINE_TYPED              50
#define UDI_DL_MOVABLE_TYPED             51
#define UDI_DL_ARRAY                     52
#define UDI_DL_END                       0
/**
 * \}
 */


typedef struct udi_init_s		udi_init_t;
typedef struct udi_primary_init_s	udi_primary_init_t;
typedef struct udi_secondary_init_s	udi_secondary_init_t;
typedef struct udi_ops_init_s	udi_ops_init_t;
typedef struct udi_cb_init_s	udi_cb_init_t;
typedef struct udi_cb_select_s	udi_cb_select_t;
typedef struct udi_gcb_init_s	udi_gcb_init_t;

typedef struct udi_init_context_s	udi_init_context_t;
typedef struct udi_limits_s		udi_limits_t;
typedef struct udi_chan_context_s	udi_chan_context_t;
typedef struct udi_child_chan_context_s	udi_child_chan_context_t;

typedef void	udi_op_t(void);
typedef udi_op_t * const	udi_ops_vector_t;

// === INCLUDE SUB-SECTIONS ===
#include "udi/cb.h"	// Control Blocks
#include "udi/time.h"	// Timer Services
#include "udi/log.h"	// Logging
#include "udi/attr.h"	// Attributes
#include "udi/strmem.h"	// String/Memory
#include "udi/queues.h"	// Queues
#include "udi/buf.h"	// Buffers
#include "udi/mem.h"	// Memory Management
#include "udi/imc.h"	// Inter-module Communication
#include "udi/meta_mgmt.h"	// Management Metalanguage
#include "udi/meta_gio.h"	// General IO Metalanguage
#include "udi/init.h"	// Init
#include "udi/mei.h"	// Init

#endif
