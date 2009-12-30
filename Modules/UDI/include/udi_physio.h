/**
 */
#ifndef _UDI_PHYSIO_H_
#include <udi.h>

/**
 * \name Bus Operations
 * \{
 */
typedef struct {
	udi_cb_t	gcb;
}	udi_bus_bind_cb_t;

/**
 * \brief Bus Bind Control Block Group Number
 */
#define UDI_BUS_BIND_CB_NUM	1

extern void	udi_bus_bind_req(udi_bus_bind_cb_t *cb);

extern void	udi_bus_unbind_req(udi_bus_bind_cb_t *cb);
/**
 * \}
 */

#endif
