/**
 * \file cb.c
 * \author John Hodge (thePowersGang)
 * \brief Control block code
 */
#define DEBUG	1
#include <udi.h>
#include <acess.h>
#include <udi_internal.h>
#include <udi_internal_ma.h>	// for cUDI_MgmtCbInitList

// === CODE ===
udi_cb_t *udi_cb_alloc_internal_v(tUDI_MetaLang *Meta, udi_index_t MetaCBNum,
	size_t inline_size, size_t scratch_size, udi_channel_t channel)
{
	ASSERTC(MetaCBNum, <, Meta->nCbTypes);
	size_t	base = Meta->CbTypes[MetaCBNum].Size;
	ASSERTC(base, >=, sizeof(udi_cb_t));
	base -= sizeof(udi_cb_t);
	LOG("+ %i + %i + %i", base, inline_size, scratch_size);
	udi_cb_t *ret = NEW(udi_cb_t, + base + inline_size + scratch_size);
	ret->channel = channel;
	ret->scratch = (void*)ret + sizeof(udi_cb_t) + base + inline_size;
	return ret;
}
void *udi_cb_alloc_internal(tUDI_DriverInstance *Inst, udi_ubit8_t bind_cb_idx, udi_channel_t channel)
{
	const udi_cb_init_t	*cb_init;
	LOG("Inst=%p, bind_cb_idx=%i, channel=%p", Inst, bind_cb_idx, channel);
	ASSERT(Inst);
	ASSERT(Inst->Module);
	ASSERT(Inst->Module->InitInfo);
	ASSERT(Inst->Module->InitInfo->cb_init_list);
	
	for( cb_init = Inst->Module->InitInfo->cb_init_list; cb_init->cb_idx; cb_init ++ )
	{
		if( cb_init->cb_idx == bind_cb_idx )
		{
			// TODO: Get base size using meta/cbnum
			tUDI_MetaLang *metalang = UDI_int_GetMetaLang(Inst->Module, cb_init->meta_idx);
			if( !metalang ) {
				Log_Warning("UDI", "Metalang referenced in %s CB %i is invalid (%i)",
					Inst->Module->ModuleName, bind_cb_idx, cb_init->meta_idx);
				return NULL;
			}
			return udi_cb_alloc_internal_v(metalang, cb_init->meta_cb_num,
				cb_init->inline_size, cb_init->scratch_requirement, channel);
			#if 0
			ASSERTC(cb_init->meta_cb_num, <, metalang->nCbTypes);
			size_t	base = metalang->CbTypes[cb_init->meta_cb_num].Size;
			ASSERTC(base, >=, sizeof(udi_cb_t));
			base -= sizeof(udi_cb_t);
			LOG("+ %i + %i + %i", base, cb_init->inline_size, cb_init->scratch_requirement);
			udi_cb_t *ret = NEW(udi_cb_t, + base
				+ cb_init->inline_size + cb_init->scratch_requirement);
			ret->channel = channel;
			ret->scratch = (void*)ret + sizeof(udi_cb_t) + base + cb_init->inline_size;
			return ret;
			#endif
		}
	}
	Log_Warning("UDI", "Cannot find CB init def %i for '%s'",
		bind_cb_idx, Inst->Module->ModuleName);
	return NULL;
}

void udi_cb_alloc (
	udi_cb_alloc_call_t	*callback,	//!< Function to be called when the CB is allocated
	udi_cb_t	*gcb,	//!< Parent Control Block
	udi_index_t	cb_idx,
	udi_channel_t	default_channel
	)
{
	tUDI_DriverInstance *inst = UDI_int_ChannelGetInstance(gcb, false, NULL);
	void *ret = udi_cb_alloc_internal(inst, cb_idx, default_channel);
	callback(gcb, ret);
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
	udi_cb_alloc_batch_call_t	*callback,	//!< 
	udi_cb_t	*gcb,	//!< 
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
	// TODO: Ensure that cb is inactive
	free(cb);
}

void udi_cancel(udi_cancel_call_t *callback, udi_cb_t *gcb)
{
	UNIMPLEMENTED();
}

// === EXPORTS ===
EXPORT(udi_cb_alloc);
EXPORT(udi_cb_alloc_dynamic);
EXPORT(udi_cb_alloc_batch);
EXPORT(udi_cb_free);
EXPORT(udi_cancel);
