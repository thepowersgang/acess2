/**
 * \file imc.c
 * \author John Hodge (thePowersGang)
 */
#define DEBUG	1
#include <acess.h>
#include <udi.h>
#include "../udi_internal.h"

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
	UNIMPLEMENTED();
}

/**
 */
extern void udi_channel_spawn(
	udi_channel_spawn_call_t *callback, udi_cb_t *gcb,
	udi_channel_t channel, udi_index_t spawn_idx,
	udi_index_t ops_idx, void *channel_context
	)
{
	LOG("gcb=%p,channel=%p", gcb, channel, spawn_idx, ops_idx, channel_context);
	
	// Search existing channel for a matching spawn_idx
	udi_channel_t ret = UDI_CreateChannel_Linked(channel, spawn_idx);

	// Bind local end of channel to ops_idx (with channel_context)
	if( ops_idx != 0 )
	{
		udi_index_t	region_idx;
		tUDI_DriverInstance	*inst = UDI_int_ChannelGetInstance(gcb, false, &region_idx);
		UDI_BindChannel(ret, false, inst, ops_idx, region_idx, channel_context, false,0);
	}
	else
	{
		// leave unbound
	}

	callback(gcb, ret);
}

/**
 * 
 */
void udi_channel_set_context(
	udi_channel_t target_channel, void *channel_context
	)
{
	LOG("target_channel=%p,channel_context=%p", target_channel, channel_context);
	UDI_int_ChannelSetContext(target_channel, channel_context);
}

void udi_channel_op_abort(
	udi_channel_t target_channel, udi_cb_t *orig_cb
	)
{
	udi_channel_event_cb_t cb;
	cb.gcb.channel = target_channel;
	cb.event = UDI_CHANNEL_CLOSED;
	cb.params.orig_cb = orig_cb;
	udi_channel_event_ind(&cb);
}

void udi_channel_close(udi_channel_t channel)
{
	Warning("%s Unimplemented", __func__);
}

void udi_channel_event_ind(udi_channel_event_cb_t *cb)
{
	LOG("cb=%p{...}", cb);
	const struct {
		udi_channel_event_ind_op_t *channel_event_ind_op;
	} *ops = UDI_int_ChannelPrepForCall( UDI_GCB(cb), NULL, 0 );
	if(!ops) {
		Log_Warning("UDI", "udi_channel_event_ind on wrong channel type");
		return ;
	}

	// UDI_int_MakeDeferredCb( UDI_GCB(cb), ops->channel_event_ind_op );

	UDI_int_ChannelReleaseFromCall( UDI_GCB(cb) );	
	ops->channel_event_ind_op(cb);
}

void udi_channel_event_complete(udi_channel_event_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}
