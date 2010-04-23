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
 * Writes to the driver's file while in component colour modes
 * must correspond to a change of the contents of the screen. The framebuffer
 * must start at offset 0 in the file.
 * In pallete colour modes the LFB is preceded by a 1024 byte pallete (allowing
 * room for 256 entries of 32-bits each)
 * Reading from the screen must either return zero, or read from the
 * framebuffer.
 * 
 * \section Mode Support
 * All video drivers must support at least one text mode (Mode #0)
 * For each graphics mode the driver exposes, there must be a corresponding
 * text mode with the same resolution, this mode will be used when the
 * user switches to a text Virtual Terminal while in graphics mode.
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
	/**
	 * ioctl(..., int *mode)
	 * \brief Get/Set Mode
	 * \return Current mode ID or -1 on error
	 * 
	 * If \a mode is non-NULL, the current video mode is set to \a *mode.
	 * This updated ID is then returned to the user.
	 */
	VIDEO_IOCTL_GETSETMODE = 4,
	
	/**
	 * ioctl(..., tVideo_IOCtl_Mode *info)
	 * \brief Find a matching mode
	 * \return 1 if a mode was found, 0 otherwise
	 * 
	 * Using avaliable modes matching the \a bpp and \a flags fields
	 * set the \a id field to the mode id of the mode with the closest
	 * \a width and \a height.
	 */
	VIDEO_IOCTL_FINDMODE,
	
	/**
	 * ioctl(..., tVideo_IOCtl_Mode *info)
	 * \brief Get mode info
	 * \return 1 if the mode exists, 0 otherwise
	 * 
	 * Set \a info's fields to the mode specified by the \a id field.
	 */
	VIDEO_IOCTL_MODEINFO,
	
	/**
	 * ioctl(..., int *NewFormat)
	 * \brief Switches between Text, Framebuffer and 3D modes
	 * \param NewFormat	Pointer to the new format code (see eTplVideo_BufFormats)
	 * \return Original format
	 * 
	 * Enabes and disables the video text mode, changing the behavior of
	 * writes to the device file.
	 */
	VIDEO_IOCTL_SETBUFFORMAT,
	
	/**
	 * ioctl(..., tVideo_IOCtl_Pos *pos)
	 * \brief Sets the cursor position
	 * \return Boolean success
	 * 
	 * Set the text mode cursor position (if it is supported)
	 * If the \a pos is set to (-1,-1) the cursor is hidden, otherwise
	 * \a pos MUST be within the current screen size (as given by the
	 * current mode's tVideo_IOCtl_Mode.width and tVideo_IOCtl_Mode.height
	 * fields).
	 */
	VIDEO_IOCTL_SETCURSOR,
	
	/**
	 * ioctl(..., tVAddr MapTo)
	 * \brief Request access to Framebuffer
	 * \return Boolean Success
	 * 
	 * Requests the driver to allow the user direct access to the
	 * framebuffer by mapping it to the supplied address.
	 * If the driver does not allow this boolean FALSE (0) is returned,
	 * else if the call succeeds (and the framebuffer ends up mapped) boolean
	 * TRUE (1) is returned.
	 */
	VIDEO_IOCTL_REQLFB
};

/**
 * \brief Mode Structure used in IOCtl Calls
 * 
 * Defines a video mode supported by (or requested of) this driver (depending
 * on what ioctl call is used)
 */
typedef struct sVideo_IOCtl_Mode
{
	short	id;		//!< Mode ID
	Uint16	width;	//!< Width
	Uint16	height;	//!< Height
	Uint8	bpp;	//!< Bits per Unit (Character or Pixel, depending on \a flags)
	Uint8	flags;	//!< Mode Flags
}	tVideo_IOCtl_Mode;

/**
 * \brief Buffer Format Codes
 */
enum eTplVideo_BufFormats
{
	/**
	 * \brief Text Mode
	 * 
	 * The device file presents itself as an array of ::tVT_Char
	 * each describing a character cell on the screen.
	 * These cells are each \a giVT_CharWidth pixels wide and
	 * \a giVT_CharHeight high.
	 */
	VIDEO_BUFFMT_TEXT,
	/**
	 * \brief Framebuffer Mode
	 * 
	 * The device file presents as an array of 32-bpp pixels describing
	 * the entire screen. The format of a single pixel is in xRGB format
	 * (top 8 bits ignored, next 8 bits red, next 8 bits green and
	 * the bottom 8 bits blue)
	 */
	VIDEO_BUFFMT_FRAMEBUFFER,
	/**
	 * \brief 2D Accelerated Mode
	 * 
	 * The device file acts as a character device, accepting a stream of
	 * commands described in eTplVideo_2DCommands when written to.
	 */
	VIDEO_BUFFMT_2DSTREAM,
	/**
	 * \brief 3D Accelerated Mode
	 * 
	 * The device file acts as a character device, accepting a stream of
	 * commands described in eTplVideo_3DCommands when written to.
	 */
	VIDEO_BUFFMT_3DSTREAM
};

/**
 * \brief Describes a position in the video framebuffer
 */
typedef struct sVideo_IOCtl_Pos
{
	Sint16	x;	//!< X Coordinate
	Sint16	y;	//!< Y Coordinate
}	tVideo_IOCtl_Pos;

/**
 * \brief Virtual Terminal Representation of a character
 */
typedef struct sVT_Char
{
	Uint32	Ch;	//!< UTF-32 Character
	union {
		struct {
			Uint16	BGCol;	//!< 12-bit Foreground Colour
			Uint16	FGCol;	//!< 12-bit Background Colour
		};
		Uint32	Colour;	//!< Compound colour for ease of access
	};
}	tVT_Char;

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
 * \brief Driver helper that renders a character to a buffer
 * \param Codepoint	Unicode character to render
 * \param Buffer	Buffer to render to (32-bpp)
 * \param Pitch	Number of DWords per line
 * \param BGC	32-bit Background Colour
 * \param FGC	32-bit Foreground Colour
 * 
 * This function is provided to help video drivers to support a simple
 * text mode by keeping the character rendering abstracted from the driver,
 * easing the driver development and reducing code duplication.
 */
extern void	VT_Font_Render(Uint32 Codepoint, void *Buffer, int Pitch, Uint32 BGC, Uint32 FGC);
/**
 * \fn Uint32 VT_Colour12to24(Uint16 Col12)
 * \brief Converts a colour from 12bpp to 32bpp
 * \param Col12	12-bpp input colour
 * \return Expanded 32-bpp (24-bit colour) version of \a Col12
 */
extern Uint32	VT_Colour12to24(Uint16 Col12);

#endif
