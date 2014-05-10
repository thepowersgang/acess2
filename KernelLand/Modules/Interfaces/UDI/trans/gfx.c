/*
 * Acess2 UDI Layer
 * - By John Hodge (thePowersGang)
 *
 * trans/gfx.c
 * - Graphics interface
 */
#define DEBUG	1
#define UDI_VERSION 0x101
#define UDI_GFX_VERSION 0x101
#include <udi.h>
#include <udi_gfx.h>
#include <acess.h>
#include <api_drv_video.h>

// === TYPES ===
typedef struct rdata_s
{
	
} rdata_t;

// === PROTOTYPES ===
// --- Management metalang
void acessgfx_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level);
void acessgfx_devmgmt_req(udi_mgmt_cb_t *cb, udi_ubit8_t mgmt_op, udi_ubit8_t parent_ID);
void acessgfx_final_cleanup_req(udi_mgmt_cb_t *cb);
// --- GFX Binding
void acessgfx_channel_event_ind(udi_channel_event_cb_t *cb);
void acessgfx_bind_ack(udi_gfx_bind_cb_t *cb, udi_index_t sockets, udi_index_t engines, udi_status_t status);
void acessgfx_unbind_ack(udi_gfx_bind_cb_t *cb);
void acessgfx_set_connector_ack(udi_gfx_state_cb_t *cb);
void acessgfx_get_connector_ack(udi_gfx_state_cb_t *cb, udi_ubit32_t value);
void acessgfx_range_connector_ack(udi_gfx_range_cb_t *cb);
void acessgfx_set_engine_ack(udi_gfx_state_cb_t *cb);
void acessgfx_get_engine_ack(udi_gfx_state_cb_t *cb, udi_ubit32_t value);
void acessgfx_range_engine_ack(udi_gfx_range_cb_t *cb);
void acessgfx_command_ack(udi_gfx_command_cb_t *cb);

// === CODE ===
void acessgfx_usage_ind(udi_usage_cb_t *cb, udi_ubit8_t resource_level)
{
	rdata_t *rdata = UDI_GCB(cb)->context;
	
	// Set up structures that don't need interegating the card to do
	
	udi_usage_res(cb);
}

void acessgfx_channel_event_ind(udi_channel_event_cb_t *cb)
{
	rdata_t *rdata = UDI_GCB(cb)->context;
	
	switch(cb->event)
	{
	case UDI_CHANNEL_CLOSED:
		udi_channel_event_complete(cb);
		return;
	case UDI_CHANNEL_BOUND:
		rdata->active_cb = UDI_GCB(cb);
		acessgfx_channel_event_ind$bound(cb->params.parent_bound.bind_cb);
		return;
	default:
		// TODO: emit an error of some form?
		return;
	}
}

void acessgfx_channel_event_ind$bound(udi_gfx_bind_cb_t *cb)
{
	rdata_t *rdata = UDI_GCB(bind_cb)->context;
	
	// request metalanguage-level bind
	udi_gfx_bind_req(bind_cb)
	// Continued in acessgfx_bind_ack
}

void acessgfx_bind_ack(udi_gfx_bind_cb_t *cb, udi_index_t sockets, udi_index_t engines, udi_status_t status)
{
	rdata_t *rdata = UDI_GCB(bind_cb)->context;
	
	if( status != UDI_OK ) {
		// binding failed
		
		return ;
	}
	
	// 
}

// === UDI Bindings ===

