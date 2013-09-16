/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
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
	struct sUDI_ChannelSide	 Side[2];
} tUDI_Channel;

// === CODE ===
udi_channel_t UDI_CreateChannel(enum eUDI_MetaLang metalang, udi_index_t meta_ops_num,
	tUDI_DriverInstance *ThisEnd, udi_index_t ThisOpsIndex,
	tUDI_DriverInstance *OtherEnd, udi_index_t OtherOpsIndex)
{
	tUDI_Channel	*ret = NEW(tUDI_Channel,);
	ret->MetaLang = metalang;
	ret->Side[0].BackPtr = ret;
//	ret->Side[0].Ops = ThisEnd->Module->InitInfo->Op;
	ret->Side[1].BackPtr = ret;
	return (udi_channel_t)&ret->Side[0].BackPtr;
}

const void *UDI_int_ChannelPrepForCall(udi_cb_t *gcb, enum eUDI_MetaLang metalang)
{
	tUDI_Channel *ch = *(tUDI_Channel**)(gcb->channel);
	ASSERTCR(ch->MetaLang, ==, metalang, NULL);

	struct sUDI_ChannelSide	*newside = (gcb->channel == (udi_channel_t)&ch->Side[0].BackPtr ? &ch->Side[1] : &ch->Side[0]);

//	gcb->initiator_context = gcb->context;
	gcb->channel = (udi_channel_t)&newside->BackPtr;
	gcb->context = newside->Context;
	return newside->Ops;
}

