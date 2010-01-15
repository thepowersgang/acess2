/**
 * \file buf.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>

// === EXPORTS ===
EXPORT(udi_buf_copy);
EXPORT(udi_buf_write);
EXPORT(udi_buf_read);
EXPORT(udi_buf_free);

// === CODE ===
void udi_buf_copy(
	udi_buf_copy_call_t *callback,
	udi_cb_t	*gcb,
	udi_buf_t	*src_buf,
	udi_size_t	src_off,
	udi_size_t	src_len,
	udi_buf_t	*dst_buf,
	udi_size_t	dst_off,
	udi_size_t	dst_len,
	udi_buf_path_t path_handle
	)
{
	UNIMPLEMENTED();
}

void udi_buf_write(
	udi_buf_write_call_t *callback,
	udi_cb_t	*gcb,
	const void	*src_mem,
	udi_size_t	src_len,
	udi_buf_t	*dst_buf,
	udi_size_t	dst_off,
	udi_size_t	dst_len,
	udi_buf_path_t path_handle
	)
{
	UNIMPLEMENTED();
}

void udi_buf_read(
	udi_buf_t	*src_buf,
	udi_size_t	src_off,
	udi_size_t	src_len,
	void	*dst_mem )
{
	UNIMPLEMENTED();
}

void udi_buf_free(udi_buf_t *buf)
{
	UNIMPLEMENTED();
}
