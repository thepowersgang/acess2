/*
 * UDI Driver Helper Macros
 *
 * GFX-specific helpers
 */
#ifndef _HELPERS_GFX_H_
#define _HELPERS_GFX_H_

typedef struct {
	udi_index_t	op;
	udi_ubit32_t	arg_1;
	udi_ubit32_t	arg_2;
	udi_ubit32_t	arg_3;
} gfxhelpers_op_t;

typedef struct {
	udi_index_t	op_count;
	const gfxhelpers_op_t	*ops;
} gfxhelpers_op_map_t;

static inline udi_ubit32_t gfxhelpers_get_engine_op(
	const gfxhelpers_op_map_t *map,	udi_index_t index, udi_index_t prop
	)
{
	if( index >= map->op_count ) {
		return 0;
	}
	switch(prop) {
	case UDI_GFX_PROP_OPERATOR_OPCODE:	return map->ops[index].op;
	case UDI_GFX_PROP_OPERATOR_ARG_1:	return map->ops[index].arg_1;
	case UDI_GFX_PROP_OPERATOR_ARG_2:	return map->ops[index].arg_2;
	case UDI_GFX_PROP_OPERATOR_ARG_3:	return map->ops[index].arg_3;
	}
	return 0;
}

static inline void gfxhelpers_return_range_simple(
	udi_gfx_range_connector_ack_op_t *callback, udi_gfx_range_cb_t *cb,
	udi_ubit32_t min, udi_ubit32_t max, udi_ubit32_t step
	)
{
	
}

static inline void gfxhelpers_return_range_set(
	udi_gfx_range_connector_ack_op_t *callback, udi_gfx_range_cb_t *cb,
	udi_ubit32_t count, ...
	)
{
	
}

static inline void gfxhelpers_return_range_fixed(
	udi_gfx_range_connector_ack_op_t *callback, udi_gfx_range_cb_t *cb,
	udi_ubit32_t value
	)
{
	gfxhelpers_return_range_simple(callback, cb, value, value, 1);
}

#endif

