/**
 * \file udi.h
 */
#ifndef _UDI_H_
#define _UDI_H_

// Use the core acess file to use the specific size types (plus va_arg)
#include <acess.h>

#include "udi/arch/x86.h"

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


// === INCLUDE SUB-SECTIONS ===
#include "udi/cb.h"	// Control Blocks
#include "udi/log.h"	// Logging
#include "udi/attr.h"	// Attributes
#include "udi/strmem.h"	// String/Memory
#include "udi/buf.h"	// Buffers
#include "udi/mem.h"	// Memory Management
#include "udi/imc.h"	// Inter-module Communication
#include "udi/meta_mgmt.h"	// Management Metalanguage
#include "udi/meta_gio.h"	// General IO Metalanguage
#include "udi/init.h"	// Init

#endif
