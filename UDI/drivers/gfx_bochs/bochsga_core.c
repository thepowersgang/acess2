/*
 * UDI Bochs Graphics Driver
 * By John Hodge (thePowersGang)
 *
 * bochsga_core.c
 * - Core Code
 */
#define UDI_VERSION	0x101
#define UDI_GFX_VERSION	0x101
#include <udi.h>
#include <udi_physio.h>
#include <udi_gfx.h>
#define DEBUG_ENABLED	1
#include "../helpers.h"
#include "../helpers_gfx.h"
#include "bochsga_common.h"

// --- Management Metalang
void bochsga_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	rdata_t	*rdata = UDI_GCB(cb)->context;
	//udi_trace_write(rdata->init_context, UDI_TREVENT_LOCAL_PROC_ENTRY, 0, );

	// TODO: Set up region data	

	udi_usage_res(cb);
}

void bochsga_enumerate_req(udi_enumerate_cb_t *cb, udi_ubit8_t enumeration_level)
{
	rdata_t	*rdata = UDI_GCB(cb)->context;
	udi_instance_attr_list_t *attr_list = cb->attr_list;
	
	switch(enumeration_level)
	{
	case UDI_ENUMERATE_START:
	case UDI_ENUMERATE_START_RESCAN:
		cb->attr_valid_length = attr_list - cb->attr_list;
		udi_enumerate_ack(cb, UDI_ENUMERATE_OK, BOCHSGA_OPS_GFX);
		break;
	case UDI_ENUMERATE_NEXT:
		udi_enumerate_ack(cb, UDI_ENUMERATE_DONE, 0);
		break;
	}
}
void bochsga_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID)
{
}
void bochsga_final_cleanup_req(udi_mgmt_cb_t *cb)
{
}
// ---
void bochsga_bus_dev_channel_event_ind(udi_channel_event_cb_t *cb);
void bochsga_bus_dev_bus_bind_ack(udi_bus_bind_cb_t *cb, udi_dma_constraints_t dma_constraints, udi_ubit8_t perferred_endianness, udi_status_t status);
void bochsga_bus_dev_bind__pio_map(udi_cb_t *cb, udi_pio_handle_t new_pio_handle);
void bochsga_bus_dev_bind__intr_chanel(udi_cb_t *gcb, udi_channel_t new_channel);
void bochsga_bus_dev_bus_unbind_ack(udi_bus_bind_cb_t *cb);

void bochsga_bus_dev_channel_event_ind(udi_channel_event_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	switch(cb->event)
	{
	case UDI_CHANNEL_CLOSED:
		break;
	case UDI_CHANNEL_BOUND: {
		rdata->active_cb = gcb;
		udi_bus_bind_cb_t *bus_bind_cb = UDI_MCB(cb->params.parent_bound.bind_cb, udi_bus_bind_cb_t);
		udi_bus_bind_req( bus_bind_cb );
		// continue at bochsga_bus_dev_bus_bind_ack
		return; }
	}
}
void bochsga_bus_dev_bus_bind_ack(udi_bus_bind_cb_t *cb,
	udi_dma_constraints_t dma_constraints, udi_ubit8_t perferred_endianness, udi_status_t status)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	// Set up PIO handles
	rdata->init.pio_index = -1;
	bochsga_bus_dev_bind__pio_map(gcb, UDI_NULL_PIO_HANDLE);
	// V V V V
}
void bochsga_bus_dev_bind__pio_map(udi_cb_t *gcb, udi_pio_handle_t new_pio_handle)
{
	rdata_t	*rdata = gcb->context;
	if( rdata->init.pio_index != -1 )
	{
		rdata->pio_handles[rdata->init.pio_index] = new_pio_handle;
	}
	
	rdata->init.pio_index ++;
	if( rdata->init.pio_index < N_PIO )
	{
		const struct s_pio_ops	*ops = &bochsga_pio_ops[rdata->init.pio_index];
		udi_pio_map(bochsga_bus_dev_bind__pio_map, gcb,
			ops->regset_idx, ops->base_offset, ops->length,
			ops->trans_list, ops->list_length,
			UDI_PIO_LITTLE_ENDIAN, 0, 0
			);
		return ;
	}
	
	udi_channel_event_complete( UDI_MCB(rdata->active_cb, udi_channel_event_cb_t), UDI_OK);
	// = = = = =
}
void bochsga_bus_dev_bus_unbind_ack(udi_bus_bind_cb_t *cb)
{
}
void bochsga_bus_dev_intr_attach_ack(udi_intr_attach_cb_t *intr_attach_cb, udi_status_t status)
{
}
void bochsga_bus_dev_intr_detach_ack(udi_intr_detach_cb_t *intr_detach_cb)
{
}
// ---
// GFX Provider ops
void bochsga_gfx_channel_event_ind(udi_channel_event_cb_t *cb)
{
	// No operation
}
void bochsga_gfx_bind_req(udi_gfx_bind_cb_t *cb)
{
	// TODO: ACK bind if nothing already bound
}
void bochsga_gfx_unbind_req(udi_gfx_bind_cb_t *cb)
{
	// TODO: Release internal state?
}
void bochsga_gfx_set_connector_req$pio(udi_cb_t *gcb, udi_buf_t *new_buf, udi_status_t status, udi_ubit16_t result)
{
	udi_gfx_state_cb_t *cb = UDI_MCB(gcb, udi_gfx_state_cb_t);
	udi_gfx_set_connector_ack(cb);
}
void bochsga_gfx_set_connector_req(udi_gfx_state_cb_t *cb, udi_ubit32_t value)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	switch(cb->attribute)
	{
	case UDI_GFX_PROP_ENABLE:
		if( rdata->output_enable != !!value )
		{
			rdata->output_enable = !!value;
			udi_pio_trans(bochsga_gfx_set_connector_req$pio, gcb,
				rdata->pio_handles[BOCHSGA_PIO_ENABLE], rdata->output_enable, NULL, &rdata->outputstate);
			return ;
		}
		udi_gfx_set_connector_ack(cb);
		return;
	// Change input engine
	// - 
	case UDI_GFX_PROP_INPUT:
		if( rdata->outputstate.engine != value )
		{
			// Validate
			if( !(0 <= value && value <= N_ENGINES) ) {
				udi_gfx_set_connector_ack(cb /*, UDI_STAT_NOT_SUPPORTED*/);
				return ;
			}
			
			// Change saved bitdepth (requires cycling enable)
			rdata->outputstate.engine = value;
			rdata->outputstate.bitdepth = bochsga_engine_defs[value].bitdepth;
		}
		udi_gfx_set_connector_ack(cb);
		return;
	// Alter output dimensions
	case UDI_GFX_PROP_WIDTH:
		if( value % 8 != 0 ) {
			// Qemu doesn't like resolutions not a multiple of 8
			return ;
		}
		if( !(320 <= value && value <= rdata->limits.max_width) ) {
			return ;
		} 
		rdata->outputstate.width = value;
		udi_pio_trans(bochsga_gfx_set_connector_req$pio, gcb,
			rdata->pio_handles[BOCHSGA_PIO_ENABLE], rdata->output_enable, NULL, &rdata->outputstate);
		return;
	case UDI_GFX_PROP_HEIGHT:
		if( !(240 <= value && value <= rdata->limits.max_height) ) {
			return ;
		} 
		rdata->outputstate.height = value;
		udi_pio_trans(bochsga_gfx_set_connector_req$pio, gcb,
			rdata->pio_handles[BOCHSGA_PIO_ENABLE], rdata->output_enable, NULL, &rdata->outputstate);
		return;
	}
	CONTIN(bochsga_gfx_set_connector_req, udi_log_write,
		(UDI_TREVENT_LOG, UDI_LOG_INFORMATION, BOCHSGA_OPS_GFX, 0, BOCHSGA_MSGNUM_PROPUNK, __func__, cb->attribute),
		(udi_status_t status)
		);
	udi_gfx_state_cb_t	*cb = UDI_MCB(cb, udi_gfx_state_cb_t);
	udi_gfx_set_connector_ack(cb /*, UDI_STAT_NOT_SUPPORTED*/);
}
void bochsga_gfx_get_connector_req(udi_gfx_state_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	switch(cb->attribute)
	{
	case UDI_GFX_PROP_ENABLE:
		udi_gfx_get_connector_ack(cb, !!rdata->output_enable);
		return;
	case UDI_GFX_PROP_INPUT:
		udi_gfx_get_connector_ack(cb, rdata->outputstate.bitdepth/8-1);
		return;
	case UDI_GFX_PROP_WIDTH:
		udi_gfx_get_connector_ack(cb, rdata->outputstate.width);
		return;
	case UDI_GFX_PROP_HEIGHT:
		udi_gfx_get_connector_ack(cb, rdata->outputstate.height);
		return;
	case UDI_GFX_PROP_CONNECTOR_TYPE:
		udi_gfx_get_connector_ack(cb, UDI_GFX_CONNECTOR_HIDDEN);
		return;
	case UDI_GFX_PROP_SIGNAL:
		udi_gfx_get_connector_ack(cb, UDI_GFX_SIGNAL_INTEGRATED);
		return;
	}
	CONTIN(bochsga_gfx_get_connector_req, udi_log_write,
		(UDI_TREVENT_LOG, UDI_LOG_INFORMATION, BOCHSGA_OPS_GFX, 0, BOCHSGA_MSGNUM_PROPUNK, __func__, cb->attribute),
		(udi_status_t status)
		);
	udi_gfx_state_cb_t	*cb = UDI_MCB(cb, udi_gfx_state_cb_t);
	udi_gfx_get_connector_ack(cb, 0);
}
void bochsga_gfx_range_connector_req(udi_gfx_range_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	switch(cb->attribute)
	{
	case UDI_GFX_PROP_ENABLE:
		// 2 values: 0 and 1
		gfxhelpers_return_range_set(udi_gfx_range_connector_ack, cb, 2, 0, 1);
		return;
	case UDI_GFX_PROP_INPUT:
		// 0--3 with a step of 1
		gfxhelpers_return_range_simple(udi_gfx_range_connector_ack, cb, 0, 3, 1);
		return;
	case UDI_GFX_PROP_WIDTH:
		gfxhelpers_return_range_simple(udi_gfx_range_connector_ack, cb,
			BOCHSGA_MIN_WIDTH, rdata->limits.max_width, 8); // qemu restricts to 8 step
		return;
	case UDI_GFX_PROP_HEIGHT:
		gfxhelpers_return_range_simple(udi_gfx_range_connector_ack, cb,
			BOCHSGA_MIN_HEIGHT, rdata->limits.max_height, 8); // step of 8 for neatness
		return;
	case UDI_GFX_PROP_CONNECTOR_TYPE:
		gfxhelpers_return_range_fixed(udi_gfx_range_connector_ack, cb, UDI_GFX_CONNECTOR_HIDDEN);
		return;
	case UDI_GFX_PROP_SIGNAL:
		gfxhelpers_return_range_fixed(udi_gfx_range_connector_ack, cb, UDI_GFX_SIGNAL_INTEGRATED);
		return;
	}
	CONTIN(bochsga_gfx_range_connector_req, udi_log_write,
		(UDI_TREVENT_LOG, UDI_LOG_INFORMATION, BOCHSGA_OPS_GFX, 0, BOCHSGA_MSGNUM_PROPUNK, __func__, cb->attribute),
		(udi_status_t status)
		);
	udi_gfx_range_cb_t	*cb = UDI_MCB(cb, udi_gfx_range_cb_t);
	udi_gfx_range_connector_ack(cb);
}
// --- Engine Manipulation ---
void bochsga_gfx_set_engine_req(udi_gfx_state_cb_t *cb, udi_ubit32_t value)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	if( cb->subsystem >= N_ENGINES ) {
		udi_gfx_get_engine_ack(cb, 0);
		return;
	}
	
	engine_t *engine = &rdata->engines[cb->subsystem];
	
	switch(cb->attribute)
	{
	case UDI_GFX_PROP_WIDTH:
		engine->width = value;
		udi_gfx_set_engine_ack(cb);
		return;
	case UDI_GFX_PROP_HEIGHT:
		engine->height = value;
		udi_gfx_set_engine_ack(cb);
		return;
	case UDI_GFX_PROP_OPERATOR_INDEX:
		if( value >= bochsga_engine_defs[cb->subsystem].op_map.op_count ) {
			// Bad value
			udi_gfx_set_engine_ack(cb);
			return;
		}
		engine->op_idx = value;
		udi_gfx_set_engine_ack(cb);
		return;
	}
	CONTIN(bochsga_gfx_set_engine_req, udi_log_write,
		(UDI_TREVENT_LOG, UDI_LOG_INFORMATION, BOCHSGA_OPS_GFX, 0, BOCHSGA_MSGNUM_PROPUNK, __func__, cb->attribute),
		(udi_status_t status)
		);
	udi_gfx_state_cb_t	*cb = UDI_MCB(cb, udi_gfx_state_cb_t);
	udi_gfx_set_engine_ack(cb);
}
void bochsga_gfx_get_engine_req(udi_gfx_state_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	if( cb->subsystem >= N_ENGINES ) {
		udi_gfx_get_engine_ack(cb, 0);
		return;
	}
	
	const engine_t *engine = &rdata->engines[cb->subsystem];
	const engine_static_t *engine_def = &bochsga_engine_defs[cb->subsystem];
	
	switch(cb->attribute)
	{
	case UDI_GFX_PROP_ENABLE:
		udi_gfx_get_engine_ack(cb, 1);
		return;
	
	case UDI_GFX_PROP_INPUT:
		udi_gfx_get_engine_ack(cb, -1);
		return;
	
	case UDI_GFX_PROP_WIDTH:
		udi_gfx_get_engine_ack(cb, engine->width);
		return;
	case UDI_GFX_PROP_HEIGHT:
		udi_gfx_get_engine_ack(cb, engine->height);
		return;
	
	case UDI_GFX_PROP_OPERATOR_INDEX:
		udi_gfx_get_engine_ack(cb, engine->op_idx);
		return;
	case UDI_GFX_PROP_OPERATOR_OPCODE:
	case UDI_GFX_PROP_OPERATOR_ARG_1:
	case UDI_GFX_PROP_OPERATOR_ARG_2:
	case UDI_GFX_PROP_OPERATOR_ARG_3:
		udi_gfx_get_engine_ack(cb, gfxhelpers_get_engine_op(&engine_def->op_map, engine->op_idx, cb->attribute));
		return;
	}
	CONTIN(bochsga_gfx_get_engine_req, udi_log_write,
		(UDI_TREVENT_LOG, UDI_LOG_INFORMATION, BOCHSGA_OPS_GFX, 0, BOCHSGA_MSGNUM_PROPUNK, __func__, cb->attribute),
		(udi_status_t status)
		);
	udi_gfx_state_cb_t	*cb = UDI_MCB(cb, udi_gfx_state_cb_t);
	udi_gfx_get_engine_ack(cb, 0);
}
void bochsga_gfx_range_engine_req(udi_gfx_range_cb_t *cb)
{
	udi_cb_t	*gcb = UDI_GCB(cb);
	rdata_t	*rdata = gcb->context;
	
	if( cb->subsystem >= N_ENGINES ) {
		udi_gfx_range_engine_ack(cb);
		return;
	}
	
	engine_t *engine = &rdata->engines[cb->subsystem];
	const engine_static_t *engine_def = &bochsga_engine_defs[cb->subsystem];
	
	switch(cb->attribute)
	{
	case UDI_GFX_PROP_ENABLE:
		gfxhelpers_return_range_fixed(udi_gfx_range_engine_ack, cb, 1);
		return;
	case UDI_GFX_PROP_INPUT:
		gfxhelpers_return_range_fixed(udi_gfx_range_engine_ack, cb, -1);
		return;
	
	case UDI_GFX_PROP_OPERATOR_INDEX:
		gfxhelpers_return_range_simple(udi_gfx_range_engine_ack, cb, 0, engine->op_idx-1, 1);
		return;
	case UDI_GFX_PROP_OPERATOR_OPCODE:
	case UDI_GFX_PROP_OPERATOR_ARG_1:
	case UDI_GFX_PROP_OPERATOR_ARG_2:
	case UDI_GFX_PROP_OPERATOR_ARG_3:
		gfxhelpers_return_range_fixed(udi_gfx_range_engine_ack, cb,
			gfxhelpers_get_engine_op(&engine_def->op_map, engine->op_idx, cb->attribute));
		return;
	}
	CONTIN(bochsga_gfx_range_engine_req, udi_log_write,
		(UDI_TREVENT_LOG, UDI_LOG_INFORMATION, BOCHSGA_OPS_GFX, 0, BOCHSGA_MSGNUM_PROPUNK, __func__, cb->attribute),
		(udi_status_t status)
		);
	udi_gfx_range_cb_t *cb = UDI_MCB(cb, udi_gfx_range_cb_t);
	udi_gfx_range_engine_ack( cb );
}
void bochsga_gfx_command_req(udi_gfx_command_cb_t *cb)
{
	// Need to parse the GLX stream
}

// ====================================================================
// - Management ops
udi_mgmt_ops_t	bochsga_mgmt_ops = {
	bochsga_usage_ind,
	bochsga_enumerate_req,
	bochsga_devmgmt_req,
	bochsga_final_cleanup_req
};
udi_ubit8_t	bochsga_mgmt_op_flags[4] = {0,0,0,0};
// - Bus Ops
udi_bus_device_ops_t	bochsga_bus_dev_ops = {
	bochsga_bus_dev_channel_event_ind,
	bochsga_bus_dev_bus_bind_ack,
	bochsga_bus_dev_bus_unbind_ack,
	bochsga_bus_dev_intr_attach_ack,
	bochsga_bus_dev_intr_detach_ack
};
udi_ubit8_t	bochsga_bus_dev_ops_flags[5] = {0};
// - GFX provider ops
udi_gfx_provider_ops_t	bochsga_gfx_ops = {
	bochsga_gfx_channel_event_ind,
	bochsga_gfx_bind_req,
	bochsga_gfx_unbind_req,
	bochsga_gfx_set_connector_req,
	bochsga_gfx_set_engine_req,
	bochsga_gfx_get_connector_req,
	bochsga_gfx_get_engine_req,
	bochsga_gfx_range_connector_req,
	bochsga_gfx_range_engine_req,
	bochsga_gfx_command_req
};
udi_ubit8_t	bochsga_gfx_ops_flags[10] = {0};
// --
udi_primary_init_t	bochsga_pri_init = {
	.mgmt_ops = &bochsga_mgmt_ops,
	.mgmt_op_flags = bochsga_mgmt_op_flags,
	.mgmt_scratch_requirement = 0,
	.enumeration_attr_list_length = 0,
	.rdata_size = sizeof(rdata_t),
	.child_data_size = 0,
	.per_parent_paths = 0
};
udi_ops_init_t	bochsga_ops_list[] = {
	{
		BOCHSGA_OPS_DEV, BOCHSGA_META_BUS, UDI_BUS_DEVICE_OPS_NUM,
		0,
		(udi_ops_vector_t*)&bochsga_bus_dev_ops,
		bochsga_bus_dev_ops_flags
	},
	{
		BOCHSGA_OPS_GFX, BOCHSGA_META_GFX, UDI_GFX_PROVIDER_OPS_NUM,
		0,
		(udi_ops_vector_t*)&bochsga_gfx_ops,
		bochsga_gfx_ops_flags
	},
	{0}
};
udi_cb_init_t bochsga_cb_init_list[] = {
	{BOCHSGA_CB_BUS_BIND,    BOCHSGA_META_BUS, UDI_BUS_BIND_CB_NUM, 0, 0,NULL},
	{BOCHSGA_CB_GFX_BIND,    BOCHSGA_META_GFX, UDI_GFX_BIND_CB_NUM, 0, 0,NULL},
	{BOCHSGA_CB_GFX_STATE,   BOCHSGA_META_GFX, UDI_GFX_STATE_CB_NUM, 0, 0,NULL},
	{BOCHSGA_CB_GFX_RANGE,   BOCHSGA_META_GFX, UDI_GFX_RANGE_CB_NUM, 0, 0,NULL},
	{BOCHSGA_CB_GFX_COMMAND, BOCHSGA_META_GFX, UDI_GFX_COMMAND_CB_NUM, 0, 0,NULL},
	{0}
};
const udi_init_t	udi_init_info = {
	.primary_init_info = &bochsga_pri_init,
	.ops_init_list = bochsga_ops_list,
	.cb_init_list = bochsga_cb_init_list,
};
