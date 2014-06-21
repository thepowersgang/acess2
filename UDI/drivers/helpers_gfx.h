/*
 * UDI Driver Helper Macros
 *
 * GFX-specific helpers
 */
#ifndef _HELPERS_GFX_H_
#define _HELPERS_GFX_H_

#if __STDC_VERSION__ < 199901L
# define	inline
#endif

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

