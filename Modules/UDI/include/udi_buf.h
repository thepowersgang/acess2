/**
 * \file udi_buf.h
 */
#ifndef _UDI_BUF_H_
#define _UDI_BUF_H_


typedef struct udi_buf_s	udi_buf_t;
typedef struct udi_xfer_constraints_s	udi_xfer_constraints_t;
typedef void udi_buf_copy_call_t(udi_cb_t *gcb, udi_buf_t *new_dst_buf);
typedef void udi_buf_write_call_t(udi_cb_t *gcb, udi_buf_t *new_dst_buf);

/**
 * \brief Describes a buffer
 * \note Semi-Opaque
 */
struct udi_buf_s
{
	udi_size_t	buf_size;
	Uint8	Data[];	//!< ENVIRONMENT ONLY
};

/**
 * \brief 
 */
struct udi_xfer_constraints_s
{
	udi_ubit32_t	udi_xfer_max;
	udi_ubit32_t	udi_xfer_typical;
	udi_ubit32_t	udi_xfer_granularity;
	udi_boolean_t	udi_xfer_one_piece;
	udi_boolean_t	udi_xfer_exact_size;
	udi_boolean_t	udi_xfer_no_reorder;
};

// --- MACROS ---
/**
 * \brief Allocates a buffer
 */
#define UDI_BUF_ALLOC(callback, gcb, init_data, size, path_handle) \
	udi_buf_write(callback, gcb, init_data, size, NULL, 0, 0, path_handle)

/**
 * \brief Inserts data into a buffer
 */
#define UDI_BUF_INSERT(callback, gcb, new_data, size, dst_buf, dst_off) \
	udi_buf_write(callback, gcb, new_data, size, dst_buf, dst_off, 0, UDI_NULL_BUF_PATH)

/**
 * \brief Removes data from a buffer (data afterwards will be moved forewards)
 */
#define UDI_BUF_DELETE(callback, gcb, size, dst_buf, dst_off) \
	udi_buf_write(callback, gcb, NULL, 0, dst_buf, dst_off, size, UDI_NULL_BUF_PATH)

/**
 * \brief Duplicates \a src_buf
 */
#define UDI_BUF_DUP(callback, gcb, src_buf, path_handle) \
	udi_buf_copy(callback, gcb, src_buf, 0, (src_buf)->buf_size, NULL, 0, 0, path_handle)


/**
 * \brief Copies data from one buffer to another
 */
extern void udi_buf_copy(
	udi_buf_copy_call_t *callback,
	udi_cb_t	*gcb,
	udi_buf_t	*src_buf,
	udi_size_t	src_off,
	udi_size_t	src_len,
	udi_buf_t	*dst_buf,
	udi_size_t	dst_off,
	udi_size_t	dst_len,
	udi_buf_path_t path_handle );

/**
 * \brief Copies data from driver space to a buffer
 */
extern void udi_buf_write(
	udi_buf_write_call_t *callback,
	udi_cb_t	*gcb,
	const void	*src_mem,
	udi_size_t	src_len,
	udi_buf_t	*dst_buf,
	udi_size_t	dst_off,
	udi_size_t	dst_len,
	udi_buf_path_t path_handle
	);

/**
 * \brief Reads data from a buffer into driver space
 */
extern void udi_buf_read(
     udi_buf_t	*src_buf,
     udi_size_t	src_off,
     udi_size_t	src_len,
     void *dst_mem );

/**
 * \brief Frees a buffer
 */
extern void udi_buf_free(udi_buf_t *buf);


#endif
