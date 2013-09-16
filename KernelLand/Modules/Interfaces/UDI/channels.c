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

struct sUDI_ChannelSide {
	struct sUDI_Channel	*BackPtr;
	const void	*Ops;
	void	*Context;
};

typedef struct sUDI_Channel
{
	enum eUDI_MetaLang	MetaLang;
	udi_index_t	MetaOpsNum;
	struct sUDI_ChannelSide	 Side[2];
} tUDI_Channel;

// === CODE ===
udi_channel_t UDI_CreateChannel(enum eUDI_MetaLang metalang, udi_index_t meta_ops_num,
	tUDI_DriverInstance *ThisEnd, udi_index_t ThisOpsIndex,
	tUDI_DriverInstance *OtherEnd, udi_index_t OtherOpsIndex)
{
	tUDI_Channel	*ret = NEW(tUDI_Channel,);
	struct {
		tUDI_DriverInstance	*inst;
		udi_index_t	ops_index;
	} ends[2] = {
		{ThisEnd, ThisOpsIndex},
		{OtherEnd, OtherOpsIndex}
	};
	ret->MetaLang = metalang;
	ret->MetaOpsNum = meta_ops_num;
	for( int i = 0; i < 2; i ++ )
	{
		if( !ends[i].inst ) {
			continue ;
		}
		tUDI_DriverModule	*mod = ends[i].inst->Module;
		ret->Side[i].BackPtr = ret;
		udi_ops_init_t	*ops = mod->InitInfo->ops_init_list;;
		while( ops->ops_idx && ops->ops_idx != ends[i].ops_index )
			ops++;
		ASSERTR(ops->ops_idx, NULL);	// TODO: Pretty error
		ASSERTCR(ops->meta_idx, <, mod->nMetaLangs, NULL);
		ASSERTCR(mod->MetaLangs[ops->meta_idx], ==, metalang, NULL);
		ASSERTCR(ops->meta_ops_num, ==, meta_ops_num, NULL);
		if( ops->chan_context_size ) {
			ret->Side[i].Context = malloc(ops->chan_context_size);
		}
		ret->Side[i].Ops = ops->ops_vector;
	}
	return (udi_channel_t)&ret->Side[0].BackPtr;
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
const void *UDI_int_ChannelPrepForCall(udi_cb_t *gcb, enum eUDI_MetaLang metalang, udi_index_t meta_ops_num)
{
	tUDI_Channel *ch = *(tUDI_Channel**)(gcb->channel);
	ASSERTCR(ch->MetaLang, ==, metalang, NULL);

	struct sUDI_ChannelSide	*newside = (gcb->channel == (udi_channel_t)&ch->Side[0].BackPtr ? &ch->Side[1] : &ch->Side[0]);

//	gcb->initiator_context = gcb->context;
	gcb->channel = (udi_channel_t)&newside->BackPtr;
	gcb->context = newside->Context;
	return newside->Ops;
}

