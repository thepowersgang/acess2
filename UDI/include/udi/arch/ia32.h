
#ifndef _UDI_ARCH_x86_H_
#define _UDI_ARCH_x86_H_

typedef udi_ubit8_t	udi_boolean_t;	/* 0=False; 1..28-1=True */
#define FALSE	0
#define TRUE	1

typedef uint32_t	udi_size_t;	/* buffer size */
typedef udi_ubit8_t	udi_index_t;	/* zero-based index type */

typedef void	*_udi_handle_t;
#define	_NULL_HANDLE	((void*)0)

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

typedef int64_t	udi_timestamp_t;

#define UDI_HANDLE_IS_NULL(handle, handle_type)	(handle == NULL)
#define UDI_HANDLE_ID(handle, handle_type)	((uint32_t)handle)

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
