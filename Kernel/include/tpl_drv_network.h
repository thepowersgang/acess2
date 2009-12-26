/**
 * \file tpl_drv_network.h
 * \brief Network Interface Driver Interface Definitions
 * 
 * \section dirs VFS Layout
 * All network drivers must have the following basic VFS structure
 * The root of the driver will only contain files that are named from zero
 * upwards that represent the present network adapters that this driver
 * controls. All VFS nodes must implement ::eTplDrv_IOCtl with
 * DRV_IOCTL_TYPE returning DRV_TYPE_NETWORK.
 * The adapter nodes must also implement ::eTplNetwork_IOCtl fully
 * (unless it is noted in the ::eTplNetwork_IOCtl documentation that a
 * call is optional)
 * 
 * \section files Adapter Files
 * \subsection Reading
 * When an adapter file is read from, the driver will block the reading
 * thread until a packet arrives (if there is not already an unhandled
 * packet in the queue) this will then be read into the destination buffer.
 * If the packet does not fit in the buffer, the end of it is discarded.
 * Likewise, if the packet does not completely fill the buffer, the call
 * will still read to the buffer and then return the size of the packet.
 * \subsection Writing
 * When an adapter is written to, the data written is encoded as a packet
 * and sent, if the data is not the correct size to be sent (if the packet
 * is too small, or if it is too large) -1 should be returned and the packet
 * will not be sent.
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
	 * \return 1 on success, 0 if the file is the root, -1 on error
	 * 
	 * Copies the six byte Media Access Control (MAC) address of the
	 * adapter to the \a MAC array.
	*/
	NET_IOCTL_GETMAC = 4
};

/**
 * \brief IOCtl name strings for use with eTplDrv_IOCtl.DRV_IOCTL_LOOKUP
 */
#define	DRV_NETWORK_IOCTLNAMES	"get_mac_addr"

#endif
