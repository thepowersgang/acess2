/**
 * \file devices/terminal.h
 */
#ifndef _SYS_DEVICES_TERMINAL_H
#define _SYS_DEVICES_TERMINAL_H


enum eDrv_Terminal {
	TERM_IOCTL_MODETYPE = 4,
	TERM_IOCTL_WIDTH,
	TERM_IOCTL_HEIGHT,
	TERM_IOCTL_QUERYMODE
};


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
