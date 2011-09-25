/**
 * \file api_drv_terminal.h
 * \brief Terminal Driver Interface Definitions
*/
#ifndef _API_DRV_TERMINAL_H
#define _API_DRV_TERMINAL_H

#include <api_drv_common.h>

/**
 * \brief Common Terminal IOCtl Calls
 * \extends eTplDrv_IOCtl
 */
enum eTplTerminal_IOCtl {
	/**
	 * ioctl(..., int *mode)
	 * \brief Get/Set the current video mode type
	 * \param mode Pointer to an integer with the new mode number (or NULL)
	 *             If \a mode is non-NULL the current terminal mode is changed/updated
	 *             to the mode indicated by \a *mode
	 * \note See ::eTplTerminal_Modes
	 * \return Current/new terminal mode
	*/
	TERM_IOCTL_MODETYPE = 4,
	
	/**
	 * ioctl(..., int *width)
	 * \brief Get/set the display width
	 * \param width	Pointer to an integer containing the new width (or NULL)
	 * \return Current/new width
	 * 
	 * If \a width is non-NULL the current width is updated (but is not
	 * applied until ::TERM_IOCTL_MODETYPE is called with \a mode non-NULL.
	 */
	TERM_IOCTL_WIDTH,
	
	/**
	 * ioctl(..., int *height)
	 * \brief Get/set the display height
	 * \param height	Pointer to an integer containing the new height
	 * \return Current height
	 * 
	 * If \a height is non-NULL the current height is updated (but is not
	 * applied until ::TERM_IOCTL_MODETYPE is called with a non-NULL \a mode.
	 */
	TERM_IOCTL_HEIGHT,
	
	/**
	 * ioctl(..., tTerm_IOCtl_Mode *info)
	 * \brief Queries the current driver about it's native modes
	 * \param info	A pointer to a ::tTerm_IOCtl_Mode with \a ID set to
	 *        the mode index (or NULL)
	 * \return Number of modes
	 * 
	 * If \a info is NULL, the number of avaliable vative display modes
	 * is returned. These display modes will have sequential ID numbers
	 * from zero up to this value.
	 * 
	 * \note The id field of \a info is not for use with ::TERM_IOCTL_MODETYPE
	 *       This field is just for indexing the mode to get its information.
	 */
	TERM_IOCTL_QUERYMODE,
	
	/**
	 * ioctl(...)
	 * \brief Forces the current terminal to be shown
	 */
	TERM_IOCTL_FORCESHOW,
	
	/**
	 * ioctl(...)
	 * \brief Returns the current text cursor position
	 * \return Cursor position (as X+Y*Width)
	 */
	TERM_IOCTL_GETCURSOR
};

/**
 * \brief Virtual Terminal Mode
 * Describes a VTerm mode to the caller of ::TERM_IOCTL_QUERYMODE
 */
typedef struct sTerm_IOCtl_Mode
{
	short	ID;		//!< Zero Based index of mode
	short	DriverID;	//!< Driver's ID number (from ::tVideo_IOCtl_Mode)
	Uint16	Height;	//!< Height
	Uint16	Width;	//!< Width
	Uint8	Depth;	//!< Bits per cell
	struct {
		unsigned bText: 1;	//!< Text Mode marker
		unsigned unused:	7;
	};
}	tTerm_IOCtl_Mode;

/**
 * \brief Terminal Modes
 */
enum eTplTerminal_Modes {
	/**
	 * \brief UTF-8 Text Mode
	 * Any writes to the terminal file are treated as UTF-8 encoded
	 * strings and reads will also return UTF-8 strings.
	 */
	TERM_MODE_TEXT,
	
	/**
	 * \brief 32bpp Framebuffer
	 * Writes to the terminal file will write to the framebuffer.
	 * Reads will return UTF-32 characters
	 */
	TERM_MODE_FB,
	
	/**
	 * \brief 32bpp 2D Accellerated mode
	 * Writes to the terminal file will be read as a command stream
	 * defined in ::eTplTerminal_2D_Commands
	 */
	TERM_MODE_2DACCEL,
	
	/**
	 * \brief OpenGL 2D/3D
	 * Writes to the terminal file will send 3D commands
	 * Reads will return UTF-32 characters
	 * \note May or may not stay in the spec
	 */
	TERM_MODE_3D,
	
	/**
	 * \brief Number of terminal modes
	 */
	NUM_TERM_MODES
};

/**
 * \brief 2D Command IDs
 * \todo Complete this structure
 * 
 * Command IDs for when the terminal type is eTplTerminal_Modes.TERM_MODE_2DACCEL
 */
enum eTplTerminal_2D_Commands
{
	/**
	 * \brief No Operation - Used for padding
	 */
	TERM_2DCMD_NOP,
	
	/**
	 * (Uint16 X, Y, W, H, Uint32 Data[])
	 * \brief Blits a bitmap to the display
	 * \param X,Y	Coordinates of Top-Left corner
	 * \param W,H	Dimensions
	 * \param Data	32-bpp pixel data
	 */
	TERM_2DCMD_PUSH
};

#endif
