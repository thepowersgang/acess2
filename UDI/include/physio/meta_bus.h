/**
 * \file physio/meta_bus.h
 */
#ifndef _PHYSIO_META_BUS_H_
#define _PHYSIO_META_BUS_H_

#include <udi.h>
#include <udi_physio.h>

typedef const struct udi_bus_device_ops_s	udi_bus_device_ops_t;
typedef const struct udi_bus_bridge_ops_s	udi_bus_bridge_ops_t;
typedef struct udi_bus_bind_cb_s	udi_bus_bind_cb_t;
typedef void	udi_bus_unbind_req_op_t(udi_bus_bind_cb_t *cb);
typedef void	udi_bus_unbind_ack_op_t(udi_bus_bind_cb_t *cb);
typedef void	udi_bus_bind_req_op_t(udi_bus_bind_cb_t *cb);
typedef void	udi_bus_bind_ack_op_t(
	udi_bus_bind_cb_t	*cb,
	udi_dma_constraints_t	dma_constraints,
	udi_ubit8_t	preferred_endianness,
	udi_status_t	status
	);


struct udi_bus_device_ops_s
{
	udi_channel_event_ind_op_t	*channel_event_ind_op;
	udi_bus_bind_ack_op_t	*bus_bind_ack_op;
	udi_bus_unbind_ack_op_t	*bus_unbind_ack_op;
	udi_intr_attach_ack_op_t	*intr_attach_ack_op;
	udi_intr_detach_ack_op_t	*intr_detach_ack_op;
};
/* Bus Device Ops Vector Number */
#define UDI_BUS_DEVICE_OPS_NUM            1

struct udi_bus_bridge_ops_s
{
     udi_channel_event_ind_op_t	*channel_event_ind_op;
     udi_bus_bind_req_op_t	*bus_bind_req_op;
     udi_bus_unbind_req_op_t	*bus_unbind_req_op;
     udi_intr_attach_req_op_t	*intr_attach_req_op;
     udi_intr_detach_req_op_t	*intr_detach_req_op;
};
/* Bus Bridge Ops Vector Number */
#define UDI_BUS_BRIDGE_OPS_NUM	2

struct udi_bus_bind_cb_s
{
     udi_cb_t gcb;
};
/* Bus Bind Control Block Group Number */
#define UDI_BUS_BIND_CB_NUM              1


extern void udi_bus_bind_req(udi_bus_bind_cb_t *cb);

extern void udi_bus_bind_ack(
	udi_bus_bind_cb_t	*cb,
	udi_dma_constraints_t	dma_constraints,
	udi_ubit8_t	preferred_endianness,
	udi_status_t	status
	);
/* Values for preferred_endianness */
#define UDI_DMA_BIG_ENDIAN                (1U<<5)
#define UDI_DMA_LITTLE_ENDIAN             (1U<<6)
#define UDI_DMA_ANY_ENDIAN                (1U<<0)

extern void udi_bus_unbind_req(udi_bus_bind_cb_t *cb);
extern void udi_bus_unbind_ack(udi_bus_bind_cb_t *cb);





#endif
