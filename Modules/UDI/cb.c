/**
 * \file bc.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_cb.h>

// === EXPORTS ===
EXPORT(udi_cb_alloc);
EXPORT(udi_cb_alloc_dynamic);
EXPORT(udi_cb_alloc_batch);
EXPORT(udi_cb_free);
EXPORT(udi_cancel);

// === CODE ===
void udi_cb_alloc (
	udi_cb_alloc_call_t *callback,
	udi_cb_t *gcb,
	udi_index_t cb_idx,
	udi_channel_t default_channel
	)
{
	UNIMPLEMENTED();
}

void udi_cb_alloc_dynamic(
	udi_cb_alloc_call_t	*callback,
	udi_cb_t	*gcb,
	udi_index_t	cb_idx,
	udi_channel_t	default_channel,
	udi_size_t	inline_size,
	udi_layout_t	*inline_layout
	)
{
	UNIMPLEMENTED();
}

void udi_cb_alloc_batch(
	udi_cb_alloc_batch_call_t	*callback,
	udi_cb_t	*gcb,
	udi_index_t	cb_idx,
	udi_index_t	count,
	udi_boolean_t	with_buf,
	udi_size_t	buf_size,
	udi_buf_path_t	path_handle
	)
{
	UNIMPLEMENTED();
}

void udi_cb_free(udi_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_cancel(udi_cancel_call_t *callback, udi_cb_t *gcb)
{
	UNIMPLEMENTED();
}
