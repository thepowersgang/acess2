/**
 * \file buf.c
 * \author John Hodge (thePowersGang)
 * 
 * Buffer Manipulation
 */
#include <acess.h>
#include <udi.h>
#include <udi_internal.h>

typedef struct sUDI_BufTag
{
	struct sUDI_BufTag	*Next;
//	udi_buf_tag_t	tag;
	struct sUDI_BufSect	*Sect;
} tUDI_BufTag;

typedef struct sUDI_BufSect
{
	struct sUDI_BufSect	*Next;
	size_t	Offset;
	size_t	Length;
	size_t	Space;
	// data
} tUDI_BufSect;

typedef struct
{
	udi_buf_t	buf;
	tUDI_BufTag	*Tags;
	tUDI_BufSect	*Sections;
} tUDI_BufInt;

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

/**
 * \brief Write to a buffer
 * \param callback	Function to call once the write has completed
 * \param gcb	Control Block
 * \param src_mem	Source Data
 * \param src_len	Length of source data
 * \param dst_buf	Destination buffer
 * \param dst_off	Destination offset in the buffer
 * \param dst_len	Length of destination area
 * \param path_handle	???
 */
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
	tUDI_BufInt	*dst = (void*)dst_buf;
	if( !dst ) {
		dst = NEW(tUDI_BufInt,);
	}
	
	if( dst_len == 0 )
	{
		// Insert / Initialise
		if( src_len > 0 )
		{
			Log_Warning("UDI", "TODO: udi_buf_write - insert");
		}
		// no-op
		else
		{
		}
	}
	else
	{
		// Overwrite
		if( src_len > 0 )
		{
			Log_Warning("UDI", "TODO: udi_buf_write - overwrite");
		}
		// Delete
		else
		{
			Log_Warning("UDI", "TODO: udi_buf_write - delete");
		}
	}
	
	callback(gcb, &dst->buf);
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
