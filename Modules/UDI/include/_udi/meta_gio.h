/*
 * Acess2 UDI Support
 * _udi/meta_gio.h
 * - General IO Metalanguage
 */
#ifndef _UDI_META_GIO_H_
#define _UDI_META_GIO_H_

typedef struct {
   	udi_channel_event_ind_op_t	*channel_event_ind_op ;
   	udi_gio_bind_ack_op_t		*gio_bind_ack_op ;
   	udi_gio_unbind_ack_op_t		*gio_unbind_ack_op ;
   	udi_gio_xfer_ack_op_t		*gio_xfer_ack_op ;
   	udi_gio_xfer_nak_op_t		*gio_xfer_nak_op ;
   	udi_gio_event_ind_op_t		*gio_event_ind_op ;
} udi_gio_client_ops_t;

typedef struct {
   	udi_channel_event_ind_op_t	*channel_event_ind_op ;
   	udi_gio_bind_req_op_t		*gio_bind_req_op ;
   	udi_gio_unbind_req_op_t		*gio_unbind_req_op ;
   	udi_gio_xfer_req_op_t		*gio_xfer_req_op ;
   	udi_gio_event_res_op_t		*gio_event_res_op ;
} udi_gio_provider_ops_t ;

#endif
