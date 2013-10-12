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
void ne2k_nd_tx_tx_req__trans_done(udi_cb_t *gcb, udi_buf_t *new_buf, udi_status_t status, udi_ubit16_t res);

// === CODE ===
void ne2k_nd_tx_channel_event_ind(udi_channel_event_cb_t *cb)
{
}

void ne2k_nd_tx_tx_req(udi_nic_tx_cb_t *cb)
{
	ne2k_rdata_t	*rdata = UDI_GCB(cb)->context;
	// TODO: TX request
	udi_debug_printf("ne2k_nd_tx_tx_req: %p\n", cb);
	udi_debug_printf("- pio_handles[%i] = %p\n", NE2K_PIO_TX, rdata->pio_handles[NE2K_PIO_TX]);
	udi_debug_printf("- cb->tx_buf->buf_size = %x\n", cb->tx_buf->buf_size);
	udi_pio_trans(ne2k_nd_tx_tx_req__trans_done, UDI_GCB(cb), rdata->pio_handles[NE2K_PIO_TX],
		0, cb->tx_buf, &cb->tx_buf->buf_size);
}
void ne2k_nd_tx_tx_req__trans_done(udi_cb_t *gcb, udi_buf_t *new_buf, udi_status_t status, udi_ubit16_t res)
{
	udi_nsr_tx_rdy( UDI_MCB(gcb, udi_nic_tx_cb_t) );
}

void ne2k_nd_tx_exp_tx_req(udi_nic_tx_cb_t *cb)
{
	// TODO: "expediant" TX reqest
	ne2k_nd_tx_tx_req(cb);
}



