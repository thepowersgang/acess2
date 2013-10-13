/**
 * \file buf.c
 * \author John Hodge (thePowersGang)
 * 
 * Buffer Manipulation
 */
#define DEBUG	0
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
	size_t	RelOfs;
	size_t	Length;
	size_t	Space;
	void	*Data;
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
	udi_buf_path_t	path_handle
	)
{
	UNIMPLEMENTED();
}

tUDI_BufSect *UDI_int_BufAddSect(size_t data_len, size_t relofs, tUDI_BufSect **prevptr, const void *data,
	udi_buf_path_t path_handle
	)
{
	const int	space_atom = 1<<7;
	size_t	space = (data_len + space_atom-1) & ~(space_atom-1);
	tUDI_BufSect	*newsect = NEW(tUDI_BufSect, + space);
	newsect->RelOfs = relofs;
	newsect->Length = data_len;
	newsect->Space = space_atom;
	newsect->Data = newsect+1;
	if( !data )
		memset(newsect->Data, 0xFF, data_len);
	else
		memcpy(newsect->Data, data, data_len);
	
	newsect->Next = *prevptr;
	*prevptr = newsect;
	LOG("@0x%x : %p <= %p+%i", relofs, newsect->Data, data, data_len);
	
	return newsect;
}

udi_buf_t *_udi_buf_allocate(const void *data, udi_size_t length, udi_buf_path_t path_handle)
{
	tUDI_BufInt *buf = NEW(tUDI_BufInt,);
	udi_buf_t *ret = &buf->buf;

	if( data ) {
		UDI_int_BufAddSect(length, 0, &buf->Sections, data, path_handle);
	}
	ret->buf_size = length;

	return ret;
}

/**
 * \brief Write to a buffer
 * \param callback	Function to call once the write has completed
 * \param gcb	Control Block
 * \param src_mem	Source Data
 * \param src_len	Length of source data
 * \param dst_buf	Destination buffer
 * \param dst_off	Destination offset in the buffer
 * \param dst_len	Length of area to be replaced
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
		dst_buf = &dst->buf;
	}

	// Find dst_off segment
	tUDI_BufSect	**prevptr = &dst->Sections;
	tUDI_BufSect	*sect = dst->Sections;
	for( ; sect; prevptr = &sect->Next, sect = sect->Next )
	{
		if(sect->RelOfs >= dst_off)
			break;
		if(sect->RelOfs + sect->Length > dst_off)
			break ;
		dst_off -= sect->RelOfs + sect->Length;
	}

	// Overwrite MIN(src_len,dst_len) bytes
	// then delete/append remainder
	size_t	len = MIN(src_len,dst_len);
	src_len -= len;
	dst_len -= len;
	while( len > 0 )
	{
		// Create new section
		if( !sect || sect->RelOfs > dst_off ) {
			size_t	newsize = MIN(sect->RelOfs - dst_off, len);
			sect = UDI_int_BufAddSect(len, dst_off, prevptr, src_mem, path_handle);
			len -= newsize;
			src_mem += newsize;
			prevptr = &sect->Next;
			sect = sect->Next;
			dst_off = 0;
		}
		if( len == 0 )
			break;
		
		// Update existing section
		size_t	bytes = MIN(len, sect->Length - dst_off);
		memcpy(sect->Data + dst_off, src_mem, bytes);
		len -= bytes;
		src_mem += bytes;
		
		prevptr = &sect->Next;
		sect = sect->Next;
	}

	if( dst_len > 0 )
	{
		ASSERT(src_len == 0);
		// Delete
		while( dst_len > 0 )
		{
			if( !sect ) {
				dst_buf->buf_size = dst_off;
				dst_len = 0;
			}
			else if( sect->RelOfs > dst_off ) {
				size_t	bytes = MIN(dst_len, sect->RelOfs - dst_off);
				dst_len -= bytes;
				dst_buf->buf_size -= bytes;
				sect->RelOfs -= bytes;
			}
			else if( dst_off == 0 && sect->Length <= dst_len ) {
				// Remove entire section
				dst_len -= sect->Length;
				dst_buf->buf_size -= sect->Length;
				*prevptr = sect->Next;
				free(sect);
				
				// Next block
				sect = *prevptr;
			}
			else if( dst_off + dst_len >= sect->Length ) {
				// block truncate
				size_t	bytes = MIN(dst_len, sect->Length - dst_off);
				dst_len -= bytes;
				dst_buf->buf_size -= bytes;
				sect->Length -= bytes;
				
				// Next block
				prevptr = &sect->Next;
				sect = sect->Next;
			}
			else {
				// in-block removal (must be last)
				ASSERTC(dst_off + dst_len, <, sect->Length);
				size_t	tail = sect->Length - (dst_off + dst_len);
				memmove(sect->Data+dst_off, sect->Data+dst_off+dst_len, tail); 
				dst_buf->buf_size -= dst_len;
				sect->Length -= dst_len;
				dst_len = 0;
			}
		}
	}
	else if( src_len > 0 )
	{
		ASSERT(dst_len == 0);
		// Insert
		if( !sect || sect->RelOfs > dst_off ) {
			// Simple: Just add a new section
			UDI_int_BufAddSect(src_len, dst_off, prevptr, src_mem, path_handle);
			dst_buf->buf_size += src_len;
		}
		else if( sect->RelOfs + sect->Length == dst_off ) {
			// End of block inserts
			size_t	avail = sect->Space - sect->Length;
			if( avail ) {
				size_t	bytes = MIN(avail, src_len);
				memcpy(sect->Data + sect->Length, src_mem, bytes);
				src_mem += bytes;
				src_len -= bytes;
			}
			if( src_len ) {
				// New block(s)
				UDI_int_BufAddSect(src_len, 0, prevptr, src_mem, path_handle);
			}
			dst_buf->buf_size += src_len;
		}
		else {
			// Complex: Need to handle mid-section inserts
			Log_Warning("UDI", "TODO: udi_buf_write - mid-section inserts");
		}
	}
	else
	{
		// No-op
	}
	
	// HACK: udi_pio_trans calls this with a NULL cb, so handle that
	if( callback ) {
		callback(gcb, &dst->buf);
	}
}

void udi_buf_read(
	udi_buf_t	*src_buf,
	udi_size_t	src_off,
	udi_size_t	src_len,
	void	*dst_mem )
{
	ENTER("psrc_buf isrc_off isrc_len pdst_mem",
		src_buf, src_off, src_len, dst_mem);
	tUDI_BufInt	*src = (void*)src_buf;
	
	tUDI_BufSect	*sect = src->Sections;
	while( src_len )
	{
		for( ; sect; sect = sect->Next )
		{
			LOG("%x <= %x <= +%x", sect->RelOfs, src_off, sect->Length);
			if(sect->RelOfs >= src_off)
				break;
			if(sect->RelOfs + sect->Length > src_off)
				break;
			src_off -= sect->RelOfs + sect->Length;
		}
		if( !sect ) {
			LOG("no section");
			break;
		}
		if( src_off < sect->RelOfs ) {
			size_t	undef_len = MIN(src_len, sect->RelOfs - src_off);
			LOG("%i undef", undef_len);
			memset(dst_mem, 0xFF, undef_len);
			dst_mem += undef_len;
			src_len -= undef_len;
			src_off += undef_len;
		}
		if( src_len == 0 )
			break;
		ASSERTC(src_off, >=, sect->RelOfs);
		size_t	ofs = src_off - sect->RelOfs;
		size_t	len = MIN(src_len, sect->Length - ofs);
		LOG("%i data from %p + %i", len, sect->Data, ofs);
		memcpy(dst_mem, sect->Data+ofs, len);
		dst_mem += len;
		src_len -= len;
	
		src_off -= sect->RelOfs + sect->Length;
		sect = sect->Next;
	}
	LEAVE('-');
}

void udi_buf_free(udi_buf_t *buf)
{
	UNIMPLEMENTED();
}
