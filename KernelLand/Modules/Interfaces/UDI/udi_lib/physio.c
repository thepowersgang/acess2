/**
 * \file physio.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_physio.h>
#include <udi_internal.h>
//#include <udi_internal_physio.h>

struct udi_dma_constraints_s
{
	udi_ubit16_t	size;
	udi_ubit16_t	n_attrs;
	udi_dma_constraints_attr_spec_t	attrs[];
};

// === EXPORTS ===
EXPORT(udi_dma_constraints_attr_set);
EXPORT(udi_dma_constraints_attr_reset);
EXPORT(udi_dma_constraints_free);

// === CODE ===
void udi_dma_constraints_attr_set(udi_dma_constraints_attr_set_call_t *callback, udi_cb_t *gcb,
	udi_dma_constraints_t src_constraints,
	const udi_dma_constraints_attr_spec_t *attr_list, udi_ubit16_t list_length,
	udi_ubit8_t flags)
{
	udi_dma_constraints_t	ret;
	if( !src_constraints )
	{
		// Allocate new
		ret = NEW_wA(struct udi_dma_constraints_s, attrs, list_length);
		if(!ret)	goto alloc_error;
		ret->size = list_length;
		ret->n_attrs = 0;
	}
	else
	{
		udi_ubit8_t	bm[256/8] = {0};
		// Calculate new size
		for( int i = 0; i < src_constraints->n_attrs; i ++ ) {
			int idx = src_constraints->attrs[i].attr_type;
			bm[idx/8] |= 1 << (idx%8);
		}
		udi_ubit16_t	count = src_constraints->n_attrs;
		for( int i = 0; i < list_length; i ++ ) {
			int idx = attr_list[i].attr_type;
			if(bm[idx/8] & 1 << (idx%8))
				;
			else {
				count ++;
				bm[idx/8] |= 1 << (idx%8);
			}
		}
		
		if( flags & UDI_DMA_CONSTRAINTS_COPY )
		{
			// Duplicate
			ret = NEW_wA(struct udi_dma_constraints_s, attrs, count);
			if(!ret)	goto alloc_error;
			ret->size = count;
			ret->n_attrs = src_constraints->n_attrs;
			memcpy(ret->attrs, src_constraints->attrs,
				src_constraints->n_attrs*sizeof(ret->attrs[0]));
		}
		else
		{
			// Expand
			ret = realloc(src_constraints, sizeof(*ret)
				+ count*sizeof(ret->attrs[0]));
			if(!ret)	goto alloc_error;
			ret->size = count;
		}
	}
	
	// Begin populating
	for( int i = 0; i < list_length; i ++ )
	{
		 int	j;
		for( j = 0; j < ret->n_attrs; j ++ )
		{
			if( ret->attrs[j].attr_type == attr_list[i].attr_type ) {
				ret->attrs[j].attr_value = attr_list[i].attr_value;
				break;
			}
		}
		if( j == ret->n_attrs )
		{
			ASSERTC(ret->n_attrs, !=, ret->size);
			ret->attrs[j].attr_type = attr_list[i].attr_type;
			ret->attrs[j].attr_value = attr_list[i].attr_value;
			ret->n_attrs ++;
		}
		
	}
	
	callback(gcb, ret, UDI_OK);
	return ;
alloc_error:
	callback(gcb, NULL, UDI_STAT_RESOURCE_UNAVAIL);
}

void udi_dma_constraints_attr_reset(
	udi_dma_constraints_t	constraints,
	udi_dma_constraints_attr_t	attr_type
	)
{
	UNIMPLEMENTED();
}

void udi_dma_constraints_free(udi_dma_constraints_t constraints)
{
	free(constraints);
}
