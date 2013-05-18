/**
 * \file api_drv_common.h
 * \brief Common Driver Interface Definitions
 * \author John Hodge (thePowersGang)
 * 
 * \section Introduction
 * There are two ways Acess drivers can communicate with userspace
 * applications, both are through the VFS. The first is by exposing a
 * device as a file buffer, the second is by sending commands via the
 * ioctl() system call.
 * All drivers in Acess must at least conform to the specifcation in this
 * file (even if it is just implementing eTplDrv_IOCtl.DRV_IOCTL_TYPE and
 * returning eTplDrv_Type.DRV_TYPE_NULL, however, doing so is discouraged)
 * 
 * \section ioctls Core IOCtl calls
 * As said, the core Acess driver specifcation defines specific IOCtl calls
 * that all drivers should implement. The four core IOCtls (defined in
 * ::eTplDrv_IOCtl) allow another binary (wether it be a user-mode application
 * or another driver) to tell what type of device a driver provides, the
 * basic identifcation of the driver (4 character ID and BCD version number)
 * and be able to use externally standardised calls that may not have
 * standardised call numbers.
 * NOTE: All ioctl calls WILL return -1 if the driver ran into an error
 * of its own fault while executing the call. If the user was at fault
 * (e.g. by passing a bad buffer) the call will return -2.
 * 
 * \section types Driver Types
 * When the eTplDrv_IOCtl.DRV_IOCTL_TYPE call is made, the driver should
 * return the relevant entry in the ::eTplDrv_Type enumeration that describes
 * what sub-specifcation (and hence, what device type) it implements.
 * These device types are described in their own files, which are liked
 * from their entries in ::eTplDrv_Type.
 */
#ifndef _API_DRV_COMMON_H
#define _API_DRV_COMMON_H

#include <acess/devices.h>

/**
 * \brief eTplDrv_IOCtl.DRV_IOCTL_LOOKUP names for the core IOCtls
 * These are the official lookup names of the core calls
 */
#define	DRV_IOCTLNAMES	"type", "ident", "version", "lookup"

/**
 * \brief Helper macro for the base IOCtl calls
 * \param _type	Type number from eTplDrv_Type to return
 * \param _ident	String of max 32-characters that identifies this driver
 * \param _version	Driver's 8.8.8 BCD version number
 * \param _ioctls	Pointer to the IOCtls string array
 * \warning If you have DEBUG enabled in the calling file, this function
 *          will do LEAVE()s before returning, so make sure that the
 *          IOCtl function is ENTER()ed when using debug with this macro
 * 
 * Usage: (Id is the IOCtl call ID)
 * \code
 * switch(Id)
 * {
 * BASE_IOCTLS(DRV_TYPE_MISC, "Ident", 0x100, csaIOCtls)
 * // Your IOCtls go here, starting at index 4
 * }
 * \endcode
 */
#define BASE_IOCTLS(_type, _ident, _version, _ioctls)	\
	case DRV_IOCTL_TYPE:	LEAVE('i', (_type));	return (_type);\
	case DRV_IOCTL_IDENT: {\
		int tmp = ModUtil_SetIdent(Data, (_ident));\
		LEAVE('i', tmp);	return tmp;\
		}\
	case DRV_IOCTL_VERSION:	LEAVE('x', (_version));	return (_version);\
	case DRV_IOCTL_LOOKUP:{\
		int tmp = ModUtil_LookupString( _ioctls, (const char*)Data );\
		LEAVE('i', tmp);\
		return tmp;\
		}

#endif
