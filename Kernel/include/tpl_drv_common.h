/**
 * \file tpl_drv_common.h
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
#ifndef _TPL_COMMON_H
#define _TPL_COMMON_H

/**
 * \enum eTplDrv_IOCtl
 * \brief Common IOCtl Calls
 */
enum eTplDrv_IOCtl {
	/**
	 * ioctl(...)
	 * \brief Get driver type
	 * \return The relevant entry from ::eTplDrv_Type
	 */
	DRV_IOCTL_TYPE,
	
	/**
	 * ioctl(..., char *dest[32])
	 * \brief Get driver identifier string
	 * \return 0 on no error
	 * 
	 * This call sets the 32-byte array \a dest to the drivers 31 charater
	 * identifier. This identifier must be unique to the driver series.
	 */
	DRV_IOCTL_IDENT,
	
	/**
	 * ioctl(...)
	 * \brief Get driver version number
	 * \return 24-bit BCD version number (2.2.2)
	 * 
	 * This call returns the 6-digit (2 major, 2 minor, 2 patch) version
	 * number of the driver.
	 */
	DRV_IOCTL_VERSION,
	
	/**
	 * ioctl(..., char *name)
	 * \brief Get a IOCtl call ID from a symbolic name
	 * \return ID number of the call, or 0 if not found
	 * 
	 * This call allows user applications to not need to know the ID numbers
	 * of this driver's IOCtl calls by taking a string and returning the
	 * IOCtl call number associated with that method name.
	 */
	DRV_IOCTL_LOOKUP
};

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

/**
 * \enum eTplDrv_Type
 * \brief Driver Types returned by DRV_IOCTL_TYPE
 */
enum eTplDrv_Type {
	DRV_TYPE_NULL,		//!< NULL Type - Custom Interface
	DRV_TYPE_MISC,		//!< Miscelanious Compilant - Supports the core calls
	DRV_TYPE_TERMINAL,	//!< Terminal - see tpl_drv_terminal.h
	DRV_TYPE_VIDEO,		//!< Video - see tpl_drv_video.h
	DRV_TYPE_SOUND,		//!< Audio
	DRV_TYPE_DISK,		//!< Disk - see tpl_drv_disk.h
	DRV_TYPE_KEYBOARD,	//!< Keyboard - see tpl_drv_keyboard.h
	DRV_TYPE_MOUSE,		//!< Mouse
	DRV_TYPE_JOYSTICK,	//!< Joystick / Gamepad
	DRV_TYPE_NETWORK	//!< Network Device - see tpl_drv_network.h
};

#endif
