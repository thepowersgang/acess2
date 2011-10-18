/**
 * \file api_drv_video.h
 * \brief Video Driver Interface Definitions
 * \note For AcessOS Version 1
 * 
 * Video drivers extend the common driver interface api_drv_common.h
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
 * Reading from the screen must either return zero, or read from the
 * framebuffer.
 * 
 * \section Mode Support
 * All video drivers must support at least one text mode (Mode #0)
 * For each graphics mode the driver exposes, there must be a corresponding
 * text mode with the same resolution, this mode will be used when the
 * user switches to a text Virtual Terminal while in graphics mode.
 */
#ifndef _API_DRV_VIDEO_H
#define _API_DRV_VIDEO_H

#include <api_drv_common.h>

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
	 * set the \a id, \a width and \a heights fields to the closest
	 * matching mode.
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
	 * ioctl(..., tVideo_IOCtl_Bitmap *Image)
	 * \brief Sets the cursor image
	 * \return Boolean success
	 *
	 * Sets the graphics mode cursor image
	 */
	VIDEO_IOCTL_SETCURSORBITMAP
};

/**
 * \brief Symbolic names for Video IOCtls (#4 onwards)
 */
#define DRV_VIDEO_IOCTLNAMES	"getset_mode", "find_mode", "mode_info", "set_buf_format", "set_cursor", "set_cursor_bitmap"

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
	Uint8	bpp;	//!< Bits per pixel
	Uint8	flags;	//!< Mode Flags (none defined, should be zero)
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
 * \brief 2D Accellerated Video Commands
 * 
 * Commands passed in the command stream for ::VIDEO_BUFFMT_2DSTREAM
 */
enum eTplVideo_2DCommands
{
	/**
	 * \brief No Operation
	 */
	VIDEO_2DOP_NOP,
	/**
	 * \brief Fill a region
	 * \param X	Uint16 - Leftmost pixels of the region
	 * \param Y	Uint16 - Topmost pixels of the region
	 * \param W	Uint16 - Width of the region
	 * \param H	Uint16 - Height of the region
	 * \param Colour	Uint32 - Value to fill with
	 */
	VIDEO_2DOP_FILL,
	/**
	 * \brief Copy a region from one part of the framebuffer to another
	 * \param DestX	Uint16 - Leftmost pixels of the destination
	 * \param DestY	Uint16 - Topmost pixels of the destination
	 * \param SrcX	Uint16 - Leftmost pixels of the source
	 * \param SrcY	Uint16 - Topmost pixels of the source
	 * \param Width	Uint16 - Width of the region
	 * \param Height	Uint16 - Height of the region
	 */
	VIDEO_2DOP_BLIT,


	/**
	 * \brief Copy a region from video memory to the framebuffer
	 */
	VIDEO_2DOP_BLITBUF,

	/**
	 * \brief Copy and scale a region from video memory to the framebuffer
	 */
	VIDEO_2DOP_BLITSCALEBUF,

	NUM_VIDEO_2DOPS
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
 * \brief Bitmap object (out of band image)
 */
typedef struct sVideo_IOCtl_Bitmap
{
	Sint16	W;	//!< Width of image
	Sint16	H;	//!< Height of image
	Sint16	XOfs;	//!< X Offset of center
	Sint16	YOfs;	//!< Y Offset of center
	Uint32	Data[];	//!< Image data (ARGB array)
}	tVideo_IOCtl_Bitmap;

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
 * \name Font rendering
 * \{
 */
/**
 * \brief Driver helper that renders a character to a buffer
 * \param Codepoint	Unicode character to render
 * \param Buffer	Buffer to render to
 * \param Depth	Bit depth of the destination buffer
 * \param Pitch	Number of bytes per line
 * \param BGC	32-bit Background Colour
 * \param FGC	32-bit Foreground Colour
 * 
 * This function is provided to help video drivers to support a simple
 * text mode by keeping the character rendering abstracted from the driver,
 * easing the driver development and reducing code duplication.
 */
extern void	VT_Font_Render(Uint32 Codepoint, void *Buffer, int Depth, int Pitch, Uint32 BGC, Uint32 FGC);
/**
 * \fn Uint32 VT_Colour12to24(Uint16 Col12)
 * \brief Converts a colour from 12bpp to 24bpp
 * \param Col12	12-bpp input colour
 * \return Expanded 32-bpp (24-bit colour) version of \a Col12
 */
extern Uint32	VT_Colour12to24(Uint16 Col12);
/**
 * \brief Converts a colour from 12bpp to 14bpp
 * \param Col12	12-bpp input colour
 * \return 15 bits per pixel value
 */
extern Uint16	VT_Colour12to15(Uint16 Col12);
/**
 * \brief Converts a colour from 12bpp to 32bpp
 * \param Col12	12-bpp input colour
 * \param Depth	Desired bit depth
 * \return \a Depth bit number, denoting Col12
 * 
 * Expands the source colour into a \a Depth bits per pixel representation.
 * The colours are expanded with preference to Green, Blue and Red in that order
 * (so, green gets the first spare pixel, blue gets the next, and red never gets
 * the spare). \n
 * The final bit of each component is used to fill the lower bits of the output.
 */
extern Uint32	VT_Colour12toN(Uint16 Col12, int Depth);
/**
 * \}
 */

/**
 * \brief Maximum cursor width for using the DrvUtil software cursor
 */
#define DRVUTIL_MAX_CURSOR_W	32
/**
 * \brief Maximum cursor width for using the DrvUtil software cursor
 */
#define DRVUTIL_MAX_CURSOR_H	32

/**
 * \brief Framebuffer information used by all DrvUtil_Video functions
 */
typedef struct sDrvUtil_Video_BufInfo
{
	/**
	 * \brief Framebuffer virtual address
	 */
	void	*Framebuffer;
	/**
	 * \brief Bytes between the start of each line
	 */
	 int	Pitch;
	/**
	 * \brief Number of pixels in each line
	 */
	short	Width;
	/**
	 * \brief Total number of lines
	 */
	short	Height;
	/**
	 * \brief Bit depth of the framebuffer
	 */
	 int	Depth;
	
	/**
	 * \name Software cursor controls
	 * \{
	 */
	/**
	 * \brief X coordinate of the cursor
	 */
	short	CursorX;
	/**
	 * \brief Y coordinate of the cursor
	 */
	short	CursorY;

	/**
	 * \brief Cursor bitmap
	 */
	tVideo_IOCtl_Bitmap	*CursorBitmap;

	/**
	 * \brief Buffer to store the area under the cursor
	 */
	void	*CursorSaveBuf;
	/*
	 * \}
	 */
} tDrvUtil_Video_BufInfo;

/**
 * \brief Handlers for eTplVideo_2DCommands
 */
typedef struct sDrvUtil_Video_2DHandlers
{
	/**
	 * \brief No Operation, Ignored
	 * \see VIDEO_2DOP_NOP
	 */
	void	*Nop;
	/**
	 * \brief Fill a buffer region
	 * \param X	Lefthand edge
	 * \param Y	Top edge
	 * \param W	Width
	 * \param H	Height
	 * \param Colour	Colour to fill with
	 * \see VIDEO_2DOP_FILL
	 */
	void	(*Fill)(void *Ent, Uint16 X, Uint16 Y, Uint16 W, Uint16 H, Uint32 Colour);
	/**
	 * \brief Fill a buffer region
	 * \param DestX	Lefthand edge of destination
	 * \param DestY	Top edge of destination
	 * \param SrcX	Lefthand edge of source
	 * \param SrcY	Top edge of source
	 * \param W	Width
	 * \param H	Height
	 * \see VIDEO_2DOP_BLIT
	 */
	void	(*Blit)(void *Ent, Uint16 DestX, Uint16 DestY, Uint16 SrcX, Uint16 SrcY, Uint16 W, Uint16 H);
} tDrvUtil_Video_2DHandlers;

/**
 * \brief Handle a 2D operation stream for a driver
 * \param Ent	Value to pass to handlers
 * \param Buffer	Stream buffer
 * \param Length	Length of stream
 * \param Handlers	Handlers to use for the stream
 * \param SizeofHandlers	Size of \a tDrvUtil_Video_2DHandlers according
 *        to the driver. Used as version control and error avoidence.
 */
extern int	DrvUtil_Video_2DStream(void *Ent, void *Buffer, int Length,
	tDrvUtil_Video_2DHandlers *Handlers, int SizeofHandlers);

/**
 * \brief Perform write operations to a LFB
 * \param Mode	Buffer mode (see eTplVideo_BufFormats)
 * \param FBInfo	Framebuffer descriptor, see type for details
 * \param Offset	Offset provided by VFS call
 * \param Length	Length provided by VFS call
 * \param Src	Data from VFS call
 * \return Number of bytes written
 *
 * Handles all write modes in software, using the VT font calls for rendering.
 * \note Calls the cursor clear and redraw if the cursor area is touched
 */
extern int	DrvUtil_Video_WriteLFB(int Mode, tDrvUtil_Video_BufInfo *FBInfo, size_t Offset, size_t Length, void *Src);

/**
 * \name Software cursor rendering
 * \{
 */
/**
 * \brief Set the cursor bitmap for a buffer
 * \param Buf	Framebuffer descriptor
 * \param Bitmap	New cursor bitmap
 */
extern void	DrvUtil_Video_SetCursor(tDrvUtil_Video_BufInfo *Buf, tVideo_IOCtl_Bitmap *Bitmap);
/**
 * \brief Render the cursor at (\a X, \a Y)
 * \param Buf	Framebuffer descriptor, see type for details
 * \param X	X coord of the cursor
 * \param Y	Y coord of the cursor
 */
extern void	DrvUtil_Video_DrawCursor(tDrvUtil_Video_BufInfo *Buf, int X, int Y);
/**
 * \brief Removes the rendered cursor from the screen
 * \param Buf	Framebuffer descriptor, see type for details
 */
extern void	DrvUtil_Video_RemoveCursor(tDrvUtil_Video_BufInfo *Buf);
/**
 * \}
 */
#endif
