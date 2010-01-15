/**
 * \file udi.h
 */
#ifndef _UDI_ARCH_H_
#define _UDI_ARCH_H_

// Use the core acess file to use the specific size types (plus va_arg)
#include <acess.h>

typedef Sint8	udi_sbit8_t;	/* signed 8-bit: -2^7..2^7-1 */
typedef Sint16	udi_sbit16_t;	/* signed 16-bit: -2^15..2^15-1 */
typedef Sint32	udi_sbit32_t;	/* signed 32-bit: -2^31..2^31-1 */
typedef Uint8	udi_ubit8_t;	/* unsigned 8-bit: 0..28-1 */
typedef Uint16	udi_ubit16_t;	/* unsigned 16-bit: 0..216-1 */
typedef Uint32	udi_ubit32_t;	/* unsigned 32-bit: 0..232-1 */

typedef udi_ubit8_t	udi_boolean_t;	/* 0=False; 1..28-1=True */
#define FALSE	0
#define TRUE	1

typedef size_t	udi_size_t;	/* buffer size */
typedef size_t	udi_index_t;	/* zero-based index type */

typedef void	*_udi_handle_t;
#define	_NULL_HANDLE	NULL

/* Channel Handle */
typedef _udi_handle_t	*udi_channel_t;
#define UDI_NULL_CHANNEL	_NULL_HANDLE

/**
 * \brief Buffer Path
 */
typedef _udi_handle_t	udi_buf_path_t;
#define UDI_NULL_BUF_PATH	_NULL_HANDLE

typedef _udi_handle_t	udi_origin_t;
#define UDI_NULL_ORIGIN	_NULL_HANDLE

typedef Sint64	udi_timestamp_t;

#define UDI_HANDLE_IS_NULL(handle, handle_type)	(handle == NULL)
#define UDI_HANDLE_ID(handle, handle_type)	((Uint32)handle)

/**
 * \name va_arg wrapper
 * \{
 */
#define UDI_VA_ARG(pvar, type, va_code)	va_arg(pvar,type)
#define UDI_VA_UBIT8_T
#define UDI_VA_SBIT8_T
#define UDI_VA_UBIT16_T
#define UDI_VA_SBIT16_T
#define UDI_VA_UBIT32_T
#define UDI_VA_SBIT32_T
#define UDI_VA_BOOLEAN_T
#define UDI_VA_INDEX_T
#define UDI_VA_SIZE_T
#define UDI_VA_STATUS_T
#define UDI_VA_CHANNEL_T
#define UDI_VA_ORIGIN_T
#define UDI_VA_POINTER
/**
 * \}
 */

/**
 * \brief Status Type
 */
typedef udi_ubit32_t	udi_status_t;

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
