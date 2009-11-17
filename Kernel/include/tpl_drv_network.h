/**
 * \file tpl_drv_network.h
 * \brief Network Interface Driver Interface Definitions
*/
#ifndef _TPL_NETWORK_H
#define _TPL_NETWORK_H

#include <tpl_drv_common.h>

/**
 * \enum eTplNetwork_IOCtl
 * \brief Common Network IOCtl Calls
 * \extends eTplDrv_IOCtl
 */
enum eTplNetwork_IOCtl {
	/**
	 * ioctl(..., Uint8 *MAC[6])
	 * \brief Get the MAC address of the interface
	*/
	NET_IOCTL_GETMAC = 4
};

#define	DRV_NETWORK_IOCTLNAMES	"get_mac_addr"

#endif
