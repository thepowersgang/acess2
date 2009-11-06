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
 * \brief Mode Structure used in IOCtl Calls
 */
struct sVideo_IOCtl_Mode {
	short	id;		//!< Mide ID
	Uint16	width;	//!< Width
	Uint16	height;	//!< Height
	Uint8	bpp;	//!< Bits per Pixel
	Uint8	flags;	//!< Mode Flags
};
typedef struct sVideo_IOCtl_Mode	tVideo_IOCtl_Mode;	//!< Mode Type

//! \name Video Mode flags
//! \{
/**
 * \brief Text Mode Flag
 * \note A text mode should have the ::sVideo_IOCtl_Mode.bpp set to 12
 */
#define VIDEO_FLAG_TEXT	0x1
/**
 * \brief Slow (non-accellerated mode)
 */
#define VIDEO_FLAG_SLOW	0x2
//! \}

/**
 * \brief Describes a position in the video framebuffer
 */
typedef struct sVideo_IOCtl_Pos	tVideo_IOCtl_Pos;
struct sVideo_IOCtl_Pos {
	Sint16	x;	//!< X Coordinate
	Sint16	y;	//!< Y Coordinate
};

/**
 * \brief Virtual Terminal Representation of a character
 */
typedef struct sVT_Char	tVT_Char;
struct sVT_Char {
	Uint32	Ch;	//!< UTF-32 Character
	union {
		struct {
			Uint16	BGCol;	//!< 12-bit Foreground Colour
			Uint16	FGCol;	//!< 12-bit Background Colour
		};
		Uint32	Colour;	//!< Compound colour for ease of access
	};
};

/**
 * \name Basic builtin colour definitions
 * \{
 */
#define	VT_COL_BLACK	0x0000
#define	VT_COL_GREY		0x0888
#define	VT_COL_LTGREY	0x0CCC
#define	VT_COL_WHITE	0x0FFF
/**
 * \}
 */

//! \brief Defines the width of a rendered character
extern int	giVT_CharWidth;
//! \brief Defines the height of a rendered character
extern int	giVT_CharHeight;
/**
 * \fn void VT_Font_Render(Uint32 Codepoint, void *Buffer, int Pitch, Uint32 BGC, Uint32 FGC)
 * \brief Renders a character to a buffer
 * \param Codepoint	Unicode character to render
 * \param Buffer	Buffer to render to (32-bpp)
 * \param Pitch	Number of DWords per line
 * \param BGC	32-bit Background Colour
 * \param FGC	32-bit Foreground Colour
 */
extern void	VT_Font_Render(Uint32 Codepoint, void *Buffer, int Pitch, Uint32 BGC, Uint32 FGC);
/**
 * \fn Uint32 VT_Colour12to24(Uint16 Col12)
 * \brief Converts a colour from 12bpp to 32bpp
 * \param Col12	12-bpp input colour
 * \return	Expanded 32-bpp (24-bit colour) version of \a Col12
 */
extern Uint32	VT_Colour12to24(Uint16 Col12);

#endif
