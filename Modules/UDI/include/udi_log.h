/**
 * \file udi_log.h
 */
#ifndef _UDI_LOG_H_
#define _UDI_LOG_H_

// Required Files
#include "udi_cb.h"

/**
 * \brief Trace Event
 */
typedef udi_ubit32_t	udi_trevent_t;

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


#endif
