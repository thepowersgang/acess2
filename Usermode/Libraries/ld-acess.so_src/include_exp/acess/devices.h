/**
 * \file drivers.h
 */
#ifndef _SYS_DRIVERS_H
#define _SYS_DRIVERS_H

// === COMMON ===
enum eDrv_Common {
	DRV_IOCTL_TYPE,
	DRV_IOCTL_IDENT,
	DRV_IOCTL_VER,
	DRV_IOCTL_LOOKUP,
};

enum eDrv_Types {
	DRV_TYPE_NULL,		//!< NULL Type - Custom Interface
	DRV_TYPE_TERMINAL,	//!< Terminal
	DRV_TYPE_VIDEO,		//!< Video - LFB
	DRV_TYPE_SOUND,		//!< Audio
	DRV_TYPE_MOUSE,		//!< Mouse
	DRV_TYPE_JOYSTICK	//!< Joystick / Gamepad
};

// === VIDEO ===
enum eDrv_Video {
	VID_IOCTL_SETMODE = 4,
	VID_IOCTL_GETMODE,
	VID_IOCTL_FINDMODE,
	VID_IOCTL_MODEINFO,
	VID_IOCTL_REQLFB	// Request LFB
};
struct sVideo_IOCtl_Mode {
	short	id;
	Uint16	width;
	Uint16	height;
	Uint16	bpp;
};
typedef struct sVideo_IOCtl_Mode	tVideo_IOCtl_Mode;	//!< Mode Type

// === MOUSE ===
enum eDrv_Mouse {
	MSE_IOCTL_SENS = 4,
	MSE_IOCTL_MAX_X,
	MSE_IOCTL_MAX_Y
};

// === Terminal ===
#include "devices/terminal.h"

#endif
