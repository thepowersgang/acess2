/**
 * \file udi_log.h
 */
#ifndef _UDI_LOG_H_
#define _UDI_LOG_H_

/**
 * \brief Trace Event
 */
typedef udi_ubit32_t	udi_trevent_t;

/**
 * \name Values for udi_trevent_t
 * \note Taken from UDI Spec 1.01
 * \{
 */
/* Common Trace Events */
#define UDI_TREVENT_LOCAL_PROC_ENTRY	(1U<<0)
#define UDI_TREVENT_LOCAL_PROC_EXIT	(1U<<1)
#define UDI_TREVENT_EXTERNAL_ERROR	(1U<<2)
/* Common Metalanguage-Selectable Trace Events */
#define UDI_TREVENT_IO_SCHEDULED	(1U<<6)
#define UDI_TREVENT_IO_COMPLETED	(1U<<7)
/* Metalanguage-Specific Trace Events */
#define UDI_TREVENT_META_SPECIFIC_1	(1U<<11)
#define UDI_TREVENT_META_SPECIFIC_2	(1U<<12)
#define UDI_TREVENT_META_SPECIFIC_3	(1U<<13)
#define UDI_TREVENT_META_SPECIFIC_4	(1U<<14)
#define UDI_TREVENT_META_SPECIFIC_5	(1U<<15)
/* Driver-Specific Trace Events */
#define UDI_TREVENT_INTERNAL_1	(1U<<16)
#define UDI_TREVENT_INTERNAL_2	(1U<<17)
#define UDI_TREVENT_INTERNAL_3	(1U<<18)
#define UDI_TREVENT_INTERNAL_4	(1U<<19)
#define UDI_TREVENT_INTERNAL_5	(1U<<20)
#define UDI_TREVENT_INTERNAL_6	(1U<<21)
#define UDI_TREVENT_INTERNAL_7	(1U<<22)
#define UDI_TREVENT_INTERNAL_8	(1U<<23)
#define UDI_TREVENT_INTERNAL_9	(1U<<24)
#define UDI_TREVENT_INTERNAL_10	(1U<<25)
#define UDI_TREVENT_INTERNAL_11	(1U<<26)
#define UDI_TREVENT_INTERNAL_12	(1U<<27)
#define UDI_TREVENT_INTERNAL_13	(1U<<28)
#define UDI_TREVENT_INTERNAL_14	(1U<<29)
#define UDI_TREVENT_INTERNAL_15	(1U<<30)
/* Logging Event */
#define UDI_TREVENT_LOG 	(1U<<31)

/**
 * \brief Log Callback
 */
typedef void udi_log_write_call_t(udi_cb_t *gcb, udi_status_t correlated_status);

/**
 * \name Log Severities
 * \brief Values for severity
 * \{
 */
#define UDI_LOG_DISASTER	1
#define UDI_LOG_ERROR		2
#define UDI_LOG_WARNING		3
#define UDI_LOG_INFORMATION	4
/**
 * \}
 */

extern void udi_trace_write(udi_init_context_t *init_context,
	udi_trevent_t trace_event, udi_index_t meta_idx,
	udi_ubit32_t msgnum, ...);

extern void udi_log_write( udi_log_write_call_t *callback, udi_cb_t *gcb,
	udi_trevent_t trace_event, udi_ubit8_t severity, udi_index_t meta_idx, udi_status_t original_status,
	udi_ubit32_t msgnum, ... );

extern void udi_assert(udi_boolean_t expr);

extern void udi_debug_break(udi_init_context_t *init_context, const char *message);

extern void udi_debug_printf(const char *format, ...);

#endif
