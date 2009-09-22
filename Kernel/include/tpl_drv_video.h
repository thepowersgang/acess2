/**
 * \file tpl_drv_video.h
 * \brief Video Driver Interface Definitions
 * \note For AcessOS Version 1
 * 
 * Video drivers extend the common driver interface tpl_drv_common.h
 * and must support _at least_ the IOCtl numbers defined in this file
 * to be compatable with Acess.
 * 
 * \section IOCtls
 * As said, a compatable driver must implement these calls correctly,
 * but they may choose not to allow direct user access to the framebuffer.
 * 
 * \section Screen Contents
 * Reads and writes to the driver's file while in component colour modes
 * must correspond to a change of the contents of the screen. The framebuffer
 * must start at offset 0 in the file.
 * In pallete colour modes the LFB is preceded by a 1024 byte pallete (allowing
 * room for 256 entries of 32-bits each)
*/
#ifndef _TPL_VIDEO_H
#define _TPL_VIDEO_H

#include <tpl_drv_common.h>

/**
 * \enum eTplVideo_IOCtl
 * \brief Common Video IOCtl Calls
 * \extends eTplDrv_IOCtl
 */
enum eTplVideo_IOCtl {
	//! \brief Set Mode - (int mode)
	VIDEO_IOCTL_SETMODE = 4,
	//! \brief Get Mode - (int *mode)
	VIDEO_IOCTL_GETMODE,
	//! \brief Find a matching mode - (tVideo_IOCtl_Mode *info)
	VIDEO_IOCTL_FINDMODE,
	//! \brief Get mode info - (tVideo_IOCtl_Mode *info)
	VIDEO_IOCTL_MODEINFO,
	//! \brief Sets the cursor position (tVideo_IOCtl_Pos *pos)
	VIDEO_IOCTL_SETCURSOR,
	//! \brief Request access to Framebuffer - (void *dest), Return Boolean Success
	VIDEO_IOCTL_REQLFB
};

/**
 \struct sVideo_IOCtl_Mode
 \brief Mode Structure used in IOCtl Calls
*/
struct sVideo_IOCtl_Mode {
	short	id;		//!< Mide ID
	Uint16	width;	//!< Width
	Uint16	height;	//!< Height
	Uint16	bpp;	//!< Bits per Pixel
};
struct sVideo_IOCtl_Pos {
	Sint16	x;
	Sint16	y;
};
typedef struct sVideo_IOCtl_Mode	tVideo_IOCtl_Mode;	//!< Mode Type
typedef struct sVideo_IOCtl_Pos	tVideo_IOCtl_Pos;	//!< Mode Type

/**
 * \struct sVT_Char
 * \brief Virtual Terminal Representation of a character
 */
struct sVT_Char {
	Uint32	Ch;
	union {
		struct {
			Uint16	BGCol;
			Uint16	FGCol;
		};
		Uint32	Colour;
	};
};
typedef struct sVT_Char	tVT_Char;

#define	VT_COL_BLACK	0x0000
#define	VT_COL_GREY		0x0888
#define	VT_COL_LTGREY	0x0CCC
#define	VT_COL_WHITE	0x0FFF

#endif
