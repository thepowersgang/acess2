/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * udi_lib/mei.c
 * - Metalanguage-to-Environment Interface
 */
#define DEBUG	1
#include <udi.h>
#include <acess.h>
#include <udi_internal.h>

typedef struct {
	tUDI_DeferredCall	DCall;
	const udi_mei_op_template_t	*mei_op;
} tUDI_MeiCall;

// === CODE ===
void _udi_mei_call_unmarshal(tUDI_DeferredCall *DCall)
{
	tUDI_MeiCall	*Call = (void*)DCall;
	const udi_mei_op_template_t	*mei_op = Call->mei_op;
	LOG("%s backend", mei_op->op_name);
	mei_op->backend_stub(Call->DCall.Handler, Call->DCall.cb, Call+1);
}

void udi_mei_call(udi_cb_t *gcb, udi_mei_init_t *meta_info, udi_index_t meta_ops_num, udi_index_t vec_idx, ...)
{
//	ENTER("pgcb pmeta_info imeta_ops_num ivec_idx",
//		gcb, meta_info, meta_ops_num, vec_idx);
	const udi_mei_op_template_t	*mei_op;

	{
		udi_mei_ops_vec_template_t	*ops = meta_info->ops_vec_template_list;
		for( ; ops->meta_ops_num && ops->meta_ops_num != meta_ops_num; ops ++ )
			;
		if( !ops->meta_ops_num ) {
			LEAVE('-');
			return ;
		}
		mei_op = &ops->op_template_list[vec_idx-1];
	}
	
	// Check CB type
	udi_index_t	cb_type;
	tUDI_MetaLang	*metalang = UDI_int_GetCbType(gcb, &cb_type);
	if( meta_info != metalang->MeiInfo && mei_op->meta_cb_num != cb_type ) {
		Log_Warning("UDI", "%s meta cb mismatch want %p:%i got %p:%i",
			mei_op->op_name,
			meta_info, mei_op->meta_cb_num,
			metalang->MeiInfo, cb_type
			);
		ASSERTC(meta_info, ==, metalang->MeiInfo);
		ASSERTC(mei_op->meta_cb_num, ==, cb_type);
//		LEAVE('-');
		return ;
	}
	
	// Determine if indirect call is needed
	// > check stack depth?
	// > Direction?
	 int indirect_call = (mei_op->op_category == UDI_MEI_OPCAT_REQ)
		|| (mei_op->op_category == UDI_MEI_OPCAT_IND);
	
	LOG("%s %sdirect", mei_op->op_name, (indirect_call ? "in" : ""));
	
	// Check call type
	udi_op_t	*const*ops = UDI_int_ChannelPrepForCall(gcb, metalang, meta_ops_num);
	udi_op_t	*op = ops[vec_idx];
	
	// Change ownership of chained cbs
	size_t	chain_ofs = metalang->CbTypes[cb_type].ChainOfs;
	if( chain_ofs )
	{
		udi_cb_t	*chained_cb = gcb;
		while( (chained_cb = *(udi_cb_t**)((void*)chained_cb + chain_ofs)) )
			UDI_int_ChannelFlip(chained_cb);
	}
	
	if( indirect_call )
	{
		va_list	args;
		va_start(args, vec_idx);
		size_t	marshal_size = _udi_marshal_values(NULL, mei_op->marshal_layout, args);
		va_end(args);
		tUDI_MeiCall	*call = malloc(sizeof(tUDI_MeiCall) + marshal_size);
		call->DCall.Next = NULL;
		call->DCall.Unmarshal = _udi_mei_call_unmarshal;
		call->DCall.cb = gcb;
		call->DCall.Handler = op;
		call->mei_op = mei_op;
		void	*marshal_space = (void*)(call+1);
		va_start(args, vec_idx);
		_udi_marshal_values(marshal_space, mei_op->marshal_layout, args);
		va_end(args);
		
		UDI_int_AddDeferred(&call->DCall);
	}
	else
	{
		va_list	args;
		va_start(args, vec_idx);
		mei_op->direct_stub( op, gcb, args );
		va_end(args);
	}
//	LEAVE('-');
}

