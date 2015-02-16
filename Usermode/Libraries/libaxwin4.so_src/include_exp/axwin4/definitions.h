/*
 * Acess2 GUIv4 (AxWin4)
 * - By John Hodge (thePowersGang)
 *
 * axwin4/definitions.h
 * - Shared definitions (Client and server)
 */
#ifndef _LIBAXWIN4_AXWIN4_DEFINITIONS_H_
#define _LIBAXWIN4_AXWIN4_DEFINITIONS_H_

/**
 * \name Window Flags
 * \{
 */
#define AXWIN4_WNDFLAG_NODECORATE	0x01	//!< Disable automatic inclusion of window decorations
#define AXWIN4_WNDFLAG_KEEPBELOW	0x02	//!< Keep the window below all others, even when it has focus
#define AXWIN4_WNDFLAG_KEEPABOVE	0x04	//!< Keep window above all others, ecen when it loses focus
/**
 * \}
 */

/**
 * \brief Global controls
 */
enum eAxWin4_GlobalControls {
	AXWIN4_CTL_BUTTON,	//!< Standard button (possibly rounded edges)
	AXWIN4_CTL_BOX, 	//!< Grouping box in a window
	AXWIN4_CTL_TOOLBAR,	//!< Toolbar (raised region)
	AXWIN4_CTL_TEXTBOX,	//!< Text edit box
};

enum eAxWin4_GlobalFonts {
	AXWIN4_FONT_DEFAULT,	//!< Default font (usually a sans-serif)
	AXWIN4_FONT_MONOSPACE,	//!< Default monospace font
};

#endif

