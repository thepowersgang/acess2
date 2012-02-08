/**
 * \file imc.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>

// === EXPORTS ===
EXPORT(udi_channel_anchor);
EXPORT(udi_channel_spawn);
EXPORT(udi_channel_set_context);
EXPORT(udi_channel_op_abort);
EXPORT(udi_channel_close);
EXPORT(udi_channel_event_ind);
EXPORT(udi_channel_event_complete);

// === CODE ===
/**
 */
void udi_channel_anchor(
	udi_channel_anchor_call_t *callback, udi_cb_t *gcb,
	udi_channel_t channel, udi_index_t ops_idx, void *channel_context
	)
{
	Warning("%s Unimplemented", __func__);
}

/**
 */
extern void udi_channel_spawn(
	udi_channel_spawn_call_t *callback, udi_cb_t *gcb,
	udi_channel_t channel, udi_index_t spawn_idx,
	udi_index_t ops_idx, void *channel_context
	)
{
	Warning("%s Unimplemented", __func__);
}

/**
 * 
 */
void udi_channel_set_context(
	udi_channel_t target_channel, void *channel_context
	)
{
	Warning("%s Unimplemented", __func__);
}

void udi_channel_op_abort(
	udi_channel_t target_channel, udi_cb_t *orig_cb
	)
{
	Warning("%s Unimplemented", __func__);
}

void udi_channel_close(udi_channel_t channel)
{
	Warning("%s Unimplemented", __func__);
}

void udi_channel_event_ind(udi_channel_event_cb_t *cb)
{
	udi_channel_event_complete(cb, UDI_OK);
}

void udi_channel_event_complete(udi_channel_event_cb_t *cb, udi_status_t status)
{
	Warning("%s Unimplemented", __func__);
}
