
#ifndef _UDI_ARCH_x86_H_
#define _UDI_ARCH_x86_H_

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
typedef udi_ubit8_t	udi_index_t;	/* zero-based index type */

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

#endif
