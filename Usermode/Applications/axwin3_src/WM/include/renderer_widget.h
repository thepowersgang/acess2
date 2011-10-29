/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render_widget.h
 * - AxWin2 Widget port
 */
#ifndef _RENDERER_WIDGET_H_
#define _RENDERER_WIDGET_H_

#include <wm_renderer.h>

enum
{
	MSG_WIDGET_CREATE,
	MSG_WIDGET_DELETE,
	MSG_WIDGET_SETTEXT
};

enum eElementTypes
{
	ELETYPE_NONE,

	ELETYPE_WINDOW,	//!< Window root element
	
	ELETYPE_BOX,	//!< Content box (invisible in itself)
	ELETYPE_TABBAR,	//!< Tab Bar
	ELETYPE_TOOLBAR,	//!< Tool Bar
	
	ELETYPE_BUTTON,	//!< Push Button
	
	ELETYPE_TEXT,	//!< Text
	ELETYPE_IMAGE,	//!< Image
	
	ELETYPE_SPACER,	//!< Visual Spacer (horizontal / vertical rule)
	
	MAX_ELETYPES	= 0x100
};

typedef struct
{
	uint32_t	Parent;
	uint32_t	NewID;
	char	DebugName[];
} tWidgetMsg_Create;

#endif

