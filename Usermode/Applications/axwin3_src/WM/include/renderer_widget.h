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

enum eElementFlags
{
	/**
	 * \brief Rendered
	 * 
	 * If set, the element will be ignored in calculating sizes and
	 * rendering.
	 */
	ELEFLAG_NORENDER    = 0x001,
	/**
	 * \brief Element visibility
	 * 
	 * If set, the element is not drawn (but still is used for size calculations)
	 */
	ELEFLAG_INVISIBLE   = 0x002,
	
	/**
	 * \brief Position an element absulutely (ignored in size calcs)
	 */
	ELEFLAG_ABSOLUTEPOS = 0x004,
	
	/**
	 * \brief Fixed size element
	 */
	ELEFLAG_FIXEDSIZE   = 0x008,
	
	/**
	 * \brief Element "orientation"
	 * 
	 * Vertical means that the children of this element are stacked,
	 * otherwise they list horizontally
	 */
	ELEFLAG_VERTICAL    = 0x010,//	ELEFLAG_HORIZONTAL  = 0x000,
	/**
	 * \brief Action for text that overflows
	 */
	ELEFLAG_WRAP        = 0x020,//	ELEFLAG_NOWRAP      = 0x000,
	/**
	 * \brief Cross size action
	 * 
	 * If this flag is set, the element will only be as large (across
	 * its parent) as is needed to encase the contents of the element.
	 * Otherwise, the element will expand to fill all avaliable space.
	 */
	ELEFLAG_NOEXPAND    = 0x040,
	
	/**
	 * \brief With (length) size action
	 * If this flag is set, the element will only be as large as
	 * is required along it's parent
	 */
	ELEFLAG_NOSTRETCH   = 0x080,
	
	/**
	 * \brief Center alignment
	 */
	ELEFLAG_ALIGN_CENTER= 0x100,
	/**
	 * \brief Right/Bottom alignment
	 * 
	 * If set, the element aligns to the end of avaliable space (instead
	 * of the beginning)
	 */
	ELEFLAG_ALIGN_END	= 0x200
};

typedef struct
{
	uint32_t	Parent;
	uint32_t	NewID;
	char	DebugName[];
} tWidgetMsg_Create;

#endif

