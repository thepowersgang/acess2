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

typedef struct sUDI_CBHeader
{
	tUDI_MetaLang	*Metalang;
	udi_index_t	MetaCBNum;
	udi_cb_t	cb;
} tUDI_CBHeader;

// === CODE ===
tUDI_MetaLang *UDI_int_GetCbType(udi_cb_t *cb, udi_index_t *meta_cb_num)
{
	tUDI_CBHeader	*hdr = (void*)cb - offsetof(tUDI_CBHeader, cb);
	if(meta_cb_num)
		*meta_cb_num = hdr->MetaCBNum;
	return hdr->Metalang;
}

udi_cb_t *udi_cb_alloc_internal_v(tUDI_MetaLang *Meta, udi_index_t MetaCBNum,
	size_t inline_size, size_t scratch_size, udi_channel_t channel)
{
	ASSERTC(MetaCBNum, <, Meta->nCbTypes);
	size_t	base = Meta->CbTypes[MetaCBNum].Size;
	ASSERTC(base, >=, sizeof(udi_cb_t));
	base -= sizeof(udi_cb_t);
	tUDI_CBHeader	*cbhdr = NEW(tUDI_CBHeader, + base + inline_size + scratch_size);
	cbhdr->Metalang = Meta;
	cbhdr->MetaCBNum = MetaCBNum;
	udi_cb_t *ret = &cbhdr->cb;
	ret->channel = channel;
	ret->scratch = (void*)(ret + 1) + base + inline_size;
	LOG("(%s) udi_cb_t + %i + %i + %i = %p",
		Meta->Name, base, inline_size, scratch_size, ret);
	return ret;
}
void *udi_cb_alloc_internal(tUDI_DriverInstance *Inst, udi_ubit8_t bind_cb_idx, udi_channel_t channel)
{
	const udi_cb_init_t	*cb_init;
	ASSERT(Inst);
	ASSERT(Inst->Module);
	LOG("Inst=%p(%s), bind_cb_idx=%i, channel=%p",
		Inst, Inst->Module->ModuleName, bind_cb_idx, channel);
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
	tUDI_DriverInstance *inst = UDI_int_ChannelGetInstance(gcb, false, NULL);
	udi_cb_init_t	*cb_init;
	for( cb_init = inst->Module->InitInfo->cb_init_list; cb_init->cb_idx; cb_init ++ )
	{
		if( cb_init->cb_idx == cb_idx )
			break;
	}
	if( cb_init->cb_idx == 0 ) {
		callback(gcb, NULL);
		return ;
	}
	tUDI_MetaLang *metalang = UDI_int_GetMetaLang(inst->Module, cb_init->meta_idx);
	if( !metalang ) {
		Log_Warning("UDI", "Metalang referenced in %s CB %i is invalid (%i)",
			inst->Module->ModuleName, cb_idx, cb_init->meta_idx);
		callback(gcb, NULL);
		return ;
	}

	if( cb_init->meta_cb_num >= metalang->nCbTypes ) {	
		Log_Warning("UDI", "Metalang CB referenced in %s CB %i is invalid (%s %i>=%i)",
			inst->Module->ModuleName, cb_idx,
			metalang->Name, cb_init->meta_cb_num, metalang->nCbTypes);
		callback(gcb, NULL);
		return ;
	}	

	// Get chain offset and buffer offset
	size_t	buf_ofs = 0;	// TODO: Multiple buffers are to be supported
	size_t	chain_ofs = metalang->CbTypes[cb_init->meta_cb_num].ChainOfs;
	{
		udi_layout_t	*layout = metalang->CbTypes[cb_init->meta_cb_num].Layout;
		if( !layout ) {
			Log_Warning("UDI", "Metalang CB %s:%i does not define a layout. Bulk alloc impossible",
				metalang->Name, cb_init->meta_cb_num);
			callback(gcb, NULL);
			return ;
		}
		
		size_t	cur_ofs = sizeof(udi_cb_t);
		while( *layout != UDI_DL_END )
		{
			if( *layout == UDI_DL_BUF ) {
				if( buf_ofs ) {
					Log_Notice("UDI", "TODO Multiple buffers in cb_alloc_batch (%s:%i, %s:%i)",
						metalang->Name, cb_init->meta_cb_num,
						inst->Module->ModuleName, cb_idx);
				}
				buf_ofs = cur_ofs;
			}
			else {
				// No-op	
			}
			
			size_t	sz = _udi_marshal_step(NULL, 0, &layout, NULL);
			if( sz == 0 ) {
				Log_Warning("UDI", "Metalang CB %s:%i has an invalid layout",
					metalang->Name, cb_init->meta_cb_num);
				callback(gcb, NULL);
				return ;
			}
			cur_ofs += sz;
		}
	}

	// Use initiator context if there's no chain
	if( !chain_ofs ) {
		chain_ofs = offsetof(udi_cb_t, initiator_context);
		LOG("chain_ofs = %i (initiator_context)", chain_ofs);
	}
	else {
		LOG("chain_ofs = %i", chain_ofs);
	}
	LOG("buf_ofs = %i", buf_ofs);

	udi_cb_t *first_cb = NULL, *cur_cb;
	udi_cb_t **prevptr = &first_cb;
	for( int i = 0; i < count; i ++ )
	{
		cur_cb = udi_cb_alloc_internal_v(metalang, cb_init->meta_cb_num,
			cb_init->inline_size, cb_init->scratch_requirement, gcb->channel);

		// Allocate buffer
		if( with_buf && buf_ofs ) {
			udi_buf_t	*buf = _udi_buf_allocate(NULL, buf_size, path_handle);
			LOG("buf +%i = %p", buf_ofs, buf);
			*(void**)((char*)cur_cb + buf_ofs) = buf;
		}

		LOG("*%p = %p", prevptr, cur_cb);
		*prevptr = cur_cb;
		prevptr = (void*)cur_cb + chain_ofs;
	}
	*prevptr = NULL;
	
	callback(gcb, first_cb);
}

void udi_cb_free(udi_cb_t *cb)
{
	tUDI_CBHeader	*hdr = (void*)cb - offsetof(tUDI_CBHeader, cb);
	// TODO: Ensure that cb is inactive
	free(hdr);
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
