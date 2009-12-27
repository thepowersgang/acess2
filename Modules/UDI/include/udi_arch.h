/*
 * Acess2 UDI Support
 * - Architecture Dependent Definitions
 */
#ifndef _UDI_ARCH_H_
#define _UDI_ARCH_H_

//#if ARCH == x86
typedef char	udi_sbit8_t;	/* signed 8-bit: -2^7..2^7-1 */
typedef short	udi_sbit16_t;	/* signed 16-bit: -2^15..2^15-1 */
typedef long	udi_sbit32_t;	/* signed 32-bit: -2^31..2^31-1 */
typedef unsigned char	udi_ubit8_t;	/* unsigned 8-bit: 0..2^8-1 */
typedef unsigned short	udi_ubit16_t;	/* unsigned 16-bit: 0..2^16-1 */
typedef unsigned long	udi_ubit32_t;	/* unsigned 32-bit: 0..2^32-1 */
typedef udi_ubit8_t	udi_boolean_t;	/* 0=False; 1..255=True */


typedef unsigned int	udi_size_t;	/* buffer size (equiv to size_t) */
typedef unsigned int	udi_index_t;	/* zero-based index type */

typedef void*	udi_channel_t;	/* UDI_NULL_CHANNEL */
typedef void*	udi_buf_path_t;	/* UDI_NULL_BUF_PATH */
typedef void*	udi_origin_t;	/* UDI_NULL_ORIGIN */

typedef void*	udi_timestamp_t;
//#endif

#endif
