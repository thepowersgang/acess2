/**
 * Acess2 User Core
 * - By John Hodge (thePowersGang)
 *
 * acess/devices.h
 * - Core device IOCtls
 */
#ifndef _ACESS_DRIVERS_H
#define _ACESS_DRIVERS_H

#ifndef PACKED
#define PACKED	__attribute((packed))
#endif

// === COMMON ===
enum eDrv_Common {
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
	 * This call sets the 32-byte array \a dest to the drivers 31 character
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
	DRV_IOCTL_LOOKUP,

	/**
	 * \brief First non-reserved IOCtl number for driver extension
	 */
	DRV_IOCTL_USERMIN = 0x1000,
};

enum eDrv_Types {
	DRV_TYPE_NULL,		//!< NULL Type - Custom Interface (must support core calls)
	DRV_TYPE_MISC,		//!< Miscelanious Compilant - Supports the core calls
	DRV_TYPE_TERMINAL,	//!< Terminal - see api_drv_terminal.h
	DRV_TYPE_VIDEO,		//!< Video - see api_drv_video.h
	DRV_TYPE_SOUND,		//!< Audio
	DRV_TYPE_DISK,		//!< Disk - see api_drv_disk.h
	DRV_TYPE_KEYBOARD,	//!< Keyboard - see api_drv_keyboard.h
	DRV_TYPE_MOUSE,		//!< Mouse
	DRV_TYPE_JOYSTICK,	//!< Joystick / Gamepad
	DRV_TYPE_NETWORK	//!< Network Device - see api_drv_network.h
};

#endif
