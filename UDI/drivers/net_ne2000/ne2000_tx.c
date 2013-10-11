/*
 * UDI Ne2000 NIC Driver
 * By John Hodge (thePowersGang)
 *
 * ne2000_tx.c
 * - Transmit Code
 */
#include <udi.h>
#include <udi_nic.h>
#include "ne2000_common.h"

// === PROTOTYPES ===

// === CODE ===
void ne2k_nd_tx_channel_event_ind(udi_channel_event_cb_t *cb)
{
}

void ne2k_nd_tx_tx_req(udi_nic_tx_cb_t *cb)
{
	// TODO: TX request
	udi_debug_printf("ne2k_nd_tx_tx_req: %p\n", cb);
	udi_nsr_tx_rdy(cb);
}

void ne2k_nd_tx_exp_tx_req(udi_nic_tx_cb_t *cb)
{
	// TODO: "expediant" TX reqest
	ne2k_nd_tx_tx_req(cb);
}



