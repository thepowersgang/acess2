/**
 * \file logging.c
 * \author John Hodge (thePowersGang)
 */
#include <common.h>
#include <udi.h>

// === PROTOTYPES ===

// === CODE ===
void udi_log_write( udi_log_write_call_t *callback, udi_cb_t *gcb,
	udi_trevent_t trace_event, udi_ubit8_t severity, udi_index_t meta_idx,
	udi_status_t original_status, udi_ubit32_t msgnum, ... )
{
	Log("UDI Log");
}
