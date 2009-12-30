/**
 */
#ifndef _UDI_LOGGING_H
#define _UDI_LOGGING_H

typedef void udi_log_write_call_t( udi_cb_t *gcb, udi_status_t correlated_status );

extern void udi_log_write(
	udi_log_write_call_t	*callback,
	udi_cb_t		*gcb,
	udi_trevent_t	trace_event,
	udi_ubit8_t		severity,
	udi_index_t		meta_idx,
	udi_status_t	original_status,
	udi_ubit32_t	msgnum,
	...
	);

/* Values for severity */
#define UDI_LOG_DISASTER	1
#define UDI_LOG_ERROR		2
#define UDI_LOG_WARNING		3
#define UDI_LOG_INFORMATION	4

#endif
