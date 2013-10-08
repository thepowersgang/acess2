/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * channels.c
 * - Channel code
 */
#define DEBUG	0
#include <acess.h>
#include <udi.h>
#include "udi_internal.h"
/*
 * LOCK_CHANNELS
 * - Prevents multiple non-dispatched operations on one channel
 * TODO: This should actually lock the GCB, not the channel
 */
#define LOCK_CHANNELS	0

#define MAX_SPAWN_IDX	6

struct sUDI_ChannelSide {
	struct sUDI_Channel	*BackPtr;
	tUDI_DriverInstance	*Instance;
	udi_index_t	RegionIdx;
	udi_index_t	MetaOpsNum;
	const void	*Ops;
	void	*AllocatedContext;
	void	*Context;
};

typedef struct sUDI_Channel
{
	tUDI_MetaLang	*MetaLang;
	bool	Locked;
	struct sUDI_ChannelSide	 Side[2];
	struct sUDI_Channel	*SpawnBinds[MAX_SPAWN_IDX];
} tUDI_Channel;

// === CODE ===
udi_channel_t UDI_CreateChannel_Blank(tUDI_MetaLang *metalang)
{
	tUDI_Channel	*ret = NEW(tUDI_Channel,);

	ret->MetaLang = metalang;
	ret->Side[0].BackPtr = ret;	
	ret->Side[1].BackPtr = ret;	

	return (udi_channel_t)&ret->Side[0].BackPtr;
}

udi_channel_t UDI_CreateChannel_Linked(udi_channel_t orig, udi_ubit8_t spawn_idx)
{
	tUDI_Channel	*ch = *(tUDI_Channel**)orig;
	ASSERT(ch);
	ASSERTC(spawn_idx, <, MAX_SPAWN_IDX);
	// TODO: mutex
	if( ch->SpawnBinds[spawn_idx] ) {
		tUDI_Channel	*ret = ch->SpawnBinds[spawn_idx];
		ch->SpawnBinds[spawn_idx] = NULL;
		// TODO: mutex
		return (udi_channel_t)&ret->Side[1].BackPtr;
	}
	udi_channel_t ret = UDI_CreateChannel_Blank( ch->MetaLang );
	ch->SpawnBinds[spawn_idx] = *(tUDI_Channel**)ret;
	// TODO: Mutex
	return ret;
}

struct sUDI_ChannelSide *UDI_int_ChannelGetSide(udi_channel_t channel, bool other_side)
{
	tUDI_Channel *ch = *(tUDI_Channel**)channel;
	if(!ch)	return NULL;

	int side_idx = (channel == (udi_channel_t)&ch->Side[0].BackPtr) ? 0 : 1;
	if( other_side )
		side_idx = 1 - side_idx;	

	LOG("side_idx = %i, other_side=%b", side_idx, other_side);

	return &ch->Side[side_idx];
}

int UDI_BindChannel_Raw(udi_channel_t channel, bool other_side, tUDI_DriverInstance *inst, udi_index_t region_idx, udi_index_t meta_ops_num,  void *context, const void *ops)
{
	struct sUDI_ChannelSide *side = UDI_int_ChannelGetSide(channel, other_side);
	side->Instance = inst;
	side->RegionIdx = region_idx;
	side->Context = context;
	side->MetaOpsNum = meta_ops_num;
	side->Ops = ops;
	return 0;
}

int UDI_BindChannel(udi_channel_t channel, bool other_side, tUDI_DriverInstance *inst, udi_index_t ops_idx, udi_index_t region, void *context, bool is_child_bind, udi_ubit32_t child_ID)
{
	tUDI_Channel *ch = *(tUDI_Channel**)channel;
	
	tUDI_DriverRegion	*rgn = inst->Regions[region];
	
	udi_ops_init_t *ops = UDI_int_GetOps(inst, ops_idx);
	if( !ops ) {
		Log_Warning("UDI", "Ops ID invalid for '%s' (%i)", inst->Module, ops_idx);
		return 1;
	}

	tUDI_MetaLang *ops_ml = UDI_int_GetMetaLang(inst->Module, ops->meta_idx);
	if( ops_ml != ch->MetaLang ) {
		Log_Warning("UDI", "Attempt by %s to bind with mismatched channel '%s' op '%s' channel",
			inst->Module, ops_ml->Name, ch->MetaLang->Name);
		return 3;
	}

	if( context ) {
		// Use provided context pointer
	}
	else if( ops->chan_context_size )
	{
		if( is_child_bind )
			ASSERTCR( ops->chan_context_size, >=, sizeof(udi_child_chan_context_t), 4 );
		else
			ASSERTCR( ops->chan_context_size, >=, sizeof(udi_chan_context_t), 4 );
		context = malloc( ops->chan_context_size );
		((udi_chan_context_t*)context)->rdata = rgn->InitContext;
		if( is_child_bind )
			((udi_child_chan_context_t*)context)->child_ID = child_ID;
		
		// TODO: The driver may change the channel context, but this must be freed by the environment
		UDI_int_ChannelGetSide(channel, other_side)->AllocatedContext = context;
	}
	else {
		context = rgn->InitContext;
	}
	
	UDI_BindChannel_Raw(channel, other_side, inst, region, ops->meta_ops_num, context, ops->ops_vector);
	return 0;
}

tUDI_DriverInstance *UDI_int_ChannelGetInstance(udi_cb_t *gcb, bool other_side, udi_index_t *region_idx)
{
	struct sUDI_ChannelSide *side = UDI_int_ChannelGetSide(gcb->channel, other_side);
	if(region_idx)
		*region_idx = side->RegionIdx;
	return side->Instance;
}

void UDI_int_ChannelSetContext(udi_channel_t channel, void *context)
{
	struct sUDI_ChannelSide *side = UDI_int_ChannelGetSide(channel, false);
	side->Context = context;
}

/**
 * \brief Prepare a cb for a channel call
 * \param gcb	Generic control block for this request
 * \param metalang	UDI metalanguage (used for validation)
 * \return Pointer to ops list
 * \retval NULL	Metalangage validation failed
 *
 * Updates the channel and context fields of the gcb, checks the metalanguage and returns
 * the handler list for the other end of the channel.
 */
const void *UDI_int_ChannelPrepForCall(udi_cb_t *gcb, tUDI_MetaLang *metalang, udi_index_t meta_ops_num)
{
	ASSERT(gcb);
	ASSERT(gcb->channel);
	tUDI_Channel *ch = *(tUDI_Channel**)(gcb->channel);
	ASSERT(ch);

	// TODO: Allow calls without auto-lock
	#if LOCK_CHANNELS
	if( ch->Locked ) {
		Log_Warning("UDI", "Channel %s:%i used while blocked (before handler was fired)",
			ch->MetaLang->Name, meta_ops_num);
		return NULL;
	}	
	ch->Locked = true;
	#endif
	
	struct sUDI_ChannelSide	*newside = UDI_int_ChannelGetSide(gcb->channel, true);
	if( metalang == NULL )
	{
		if( ch->MetaLang == &cMetaLang_Management ) {
			Log_Warning("UDI", "Invalid udi_channel_event_ind on Management metalang");
			return NULL;
		}
	}
	else
	{
		if( ch->MetaLang != metalang || newside->MetaOpsNum != meta_ops_num ) {
			Log_Warning("UDI", "Metalanguage mismatch %s:%i req != %s:%i ch",
				metalang->Name, meta_ops_num,
				ch->MetaLang->Name, newside->MetaOpsNum);
			return NULL;
		}
	}

	gcb->channel = (udi_channel_t)&newside->BackPtr;
	gcb->context = newside->Context;
	if( !newside->Ops ) {
		Log_Warning("UDI", "Target end of %p(%s:%i) is unbound",
			ch, ch->MetaLang->Name, newside->MetaOpsNum);
	}
	return newside->Ops;
}

void UDI_int_ChannelFlip(udi_cb_t *gcb)
{
	ASSERT(gcb);
	ASSERT(gcb->channel);
	tUDI_Channel *ch = *(tUDI_Channel**)(gcb->channel);
	ASSERT(ch);
	
	struct sUDI_ChannelSide	*newside = UDI_int_ChannelGetSide(gcb->channel, true);

	gcb->channel = (udi_channel_t)&newside->BackPtr;
	gcb->context = newside->Context;
}

void UDI_int_ChannelReleaseFromCall(udi_cb_t *gcb)
{
	#if LOCK_CHANNELS
	ASSERT(gcb);
	ASSERT(gcb->channel);
	tUDI_Channel *ch = *(tUDI_Channel**)(gcb->channel);
	if( !ch ) {
		Log_Error("UDI", "Channel pointer of cb %p is NULL", gcb);
	}
	ASSERT(ch);
	
	ch->Locked = false;
	#endif
}

