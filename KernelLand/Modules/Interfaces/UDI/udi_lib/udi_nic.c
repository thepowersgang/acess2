/**
 * \file physio.c
 * \author John Hodge (thePowersGang)
 */
#include <acess.h>
#include <udi.h>
#include <udi_nic.h>

// === EXPORTS ===
EXPORT(udi_nd_bind_req);
EXPORT(udi_nsr_bind_ack);
EXPORT(udi_nd_unbind_req);
EXPORT(udi_nsr_unbind_ack);
EXPORT(udi_nd_enable_req);
EXPORT(udi_nsr_enable_ack);
EXPORT(udi_nd_disable_req);
EXPORT(udi_nd_ctrl_req);
EXPORT(udi_nsr_ctrl_ack);
EXPORT(udi_nsr_status_ind);
EXPORT(udi_nd_info_req);
EXPORT(udi_nsr_info_ack);
// - TX
EXPORT(udi_nsr_tx_rdy);
EXPORT(udi_nd_tx_req);
EXPORT(udi_nd_exp_tx_req);
// - RX
EXPORT(udi_nsr_rx_ind);
EXPORT(udi_nsr_exp_rx_ind);
EXPORT(udi_nd_rx_rdy);

// === CODE ===
// --- Control Ops ---
void udi_nd_bind_req(udi_nic_bind_cb_t *cb, udi_index_t tx_chan_index, udi_index_t rx_chan_index)
{
	UNIMPLEMENTED();
}

void udi_nsr_bind_ack(udi_nic_bind_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}

void udi_nd_unbind_req(udi_nic_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nsr_unbind_ack(udi_nic_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}

void udi_nd_enable_req(udi_nic_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nsr_enable_ack(udi_nic_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}

void udi_nd_disable_req(udi_nic_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nd_ctrl_req(udi_nic_ctrl_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nsr_ctrl_ack(udi_nic_ctrl_cb_t *cb, udi_status_t status)
{
	UNIMPLEMENTED();
}

void udi_nsr_status_ind(udi_nic_status_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nd_info_req(udi_nic_info_cb_t *cb, udi_boolean_t reset_statistics)
{
	UNIMPLEMENTED();
}

void udi_nsr_info_ack(udi_nic_info_cb_t *cb)
{
	UNIMPLEMENTED();
}

// --- TX ---
void udi_nsr_tx_rdy(udi_nic_tx_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nd_tx_req(udi_nic_tx_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nd_exp_tx_req(udi_nic_tx_cb_t *cb)
{
	UNIMPLEMENTED();
}

// --- RX ---
void udi_nsr_rx_ind(udi_nic_rx_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nsr_exp_rx_ind(udi_nic_rx_cb_t *cb)
{
	UNIMPLEMENTED();
}

void udi_nd_rx_rdy(udi_nic_rx_cb_t *cb)
{
	UNIMPLEMENTED();
}

