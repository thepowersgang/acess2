/**
 * \file tpl_drv_terminal.h
 * \brief Terminal Driver Interface Definitions
*/
#ifndef _TPL_TERMINAL_H
#define _TPL_TERMINAL_H

#include <tpl_drv_common.h>

/**
 * \enum eTplTerminal_IOCtl
 * \brief Common Terminal IOCtl Calls
 * \extends eTplDrv_IOCtl
 */
enum eTplTerminal_IOCtl {
	/**
	 * ioctl(..., int *mode)
	 * \brief Get/Set the current video mode type
	 * see ::eTplTerminal_Modes
	 * \note If the mode is set the mode is changed at this call
	*/
	TERM_IOCTL_MODETYPE = 4,
	
	/**
	 * ioctl(..., int *width)
	 * \brief Get/set the display width
	 * \param width	Pointer to an integer containing the new width
	 * \return Current width
	 * 
	 * Set `width` to NULL to just return the current width
	 */
	TERM_IOCTL_WIDTH,
	
	/**
	 * ioctl(..., int *height)
	 * \brief Get/set the display height
	 * \param height	Pointer to an integer containing the new height
	 * \return Current height
	 * 
	 * Set \a height to NULL to just return the current height
	 */
	TERM_IOCTL_HEIGHT,
	
	/**
	 * ioctl(..., tTerm_IOCtl_Mode *info)
	 * \brief Queries the current driver about it's modes
	 * \param info	A pointer to a ::tTerm_IOCtl_Mode with .ID set to the mode index
	 * \return Number of modes
	 * 
	 * \a info can be NULL
	 */
	TERM_IOCTL_QUERYMODE
};

typedef struct sTerm_IOCtl_Mode	tTerm_IOCtl_Mode;

/**
 * \brief Virtual Terminal Mode
 * Describes a VTerm mode to the caller of ::TERM_IOCTL_QUERYMODE
 */
struct sTerm_IOCtl_Mode
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
};

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
	 * \brief OpenGL 2D/3D
	 * Writes to the terminal file will send 3D commands
	 * Reads will return UTF-32 characters
	 * \note May or may not stay in the spec
	 */
	TERM_MODE_OPENGL,
	
	NUM_TERM_MODES
};


#endif
