/*
 * Acess2 GUI Version 3 (AxWin3)
 * - By John Hodge (thePowersGang)
 *
 * widget.h
 * - Server-side widget library
 */
#ifndef _AXWIN3_WIDGET_H_
#define _AXWIN3_WIDGET_H_

#include "axwin.h"

typedef struct sAxWin3_Widget	tAxWin3_Widget;

// --- Callback types
typedef int	(*tAxWin3_Widget_FireCb)(tAxWin3_Widget *Widget);
typedef int	(*tAxWin3_Widget_KeyUpDownCb)(tAxWin3_Widget *Widget, int KeySym);
typedef int	(*tAxWin3_Widget_KeyFireCb)(tAxWin3_Widget *Widget, int KeySym, int Character);
typedef int	(*tAxWin3_Widget_MouseMoveCb)(tAxWin3_Widget *Widget, int X, int Y);
typedef int	(*tAxWin3_Widget_MouseBtnCb)(tAxWin3_Widget *Widget, int X, int Y, int Button, int bPressed);

// --- Windows
extern tHWND	AxWin3_Widget_CreateWindow(tHWND Parent, int W, int H, int RootEleFlags);
extern void	AxWin3_Widget_DestroyWindow(tHWND Window);
extern tAxWin3_Widget	*AxWin3_Widget_GetRoot(tHWND Window);

// --- Element Creation
extern tAxWin3_Widget	*AxWin3_Widget_AddWidget(tAxWin3_Widget *Parent, int Type, int Flags, const char *DebugName);
extern void	AxWin3_Widget_DelWidget(tAxWin3_Widget *Widget);

// --- Callbacks
extern void	AxWin3_Widget_SetFireHandler(tAxWin3_Widget *Widget, tAxWin3_Widget_FireCb Callback);
extern void	AxWin3_Widget_SetKeyHandler(tAxWin3_Widget *Widget, tAxWin3_Widget_KeyUpDownCb Callback);
extern void	AxWin3_Widget_SetKeyFireHandler(tAxWin3_Widget *Widget, tAxWin3_Widget_KeyFireCb Callback);
extern void	AxWin3_Widget_SetMouseMoveHandler(tAxWin3_Widget *Widget, tAxWin3_Widget_MouseMoveCb Callback);
extern void	AxWin3_Widget_SetMouseButtonHandler(tAxWin3_Widget *Widget, tAxWin3_Widget_MouseBtnCb Callback);
// --- Manipulation
extern void	AxWin3_Widget_SetFlags(tAxWin3_Widget *Widget, int FlagSet, int FlagMask);
extern void	AxWin3_Widget_SetSize(tAxWin3_Widget *Widget, int Size);
extern void	AxWin3_Widget_SetText(tAxWin3_Widget *Widget, const char *Text);
extern void	AxWin3_Widget_SetColour(tAxWin3_Widget *Widget, int Index, tAxWin3_Colour Colour);
// --- Inspection
extern char	*AxWin3_Widget_GetText(tAxWin3_Widget *Widget);

enum eElementTypes
{
	ELETYPE_NONE,

	ELETYPE_SUBWIN,

	ELETYPE_BOX,	//!< Content box (invisible in itself)
	ELETYPE_TEXT,	//!< Text
	ELETYPE_IMAGE,	//!< Image
	ELETYPE_BUTTON,	//!< Push Button
	ELETYPE_SPACER,	//!< Visual Spacer (horizontal / vertical rule)
	ELETYPE_TEXTINPUT,	//!< Text Input Field
	ELETYPE_TEXTBOX,	//!< Text Box Input

	ELETYPE_TABBAR,	//!< Tab Bar
	ELETYPE_TOOLBAR,	//!< Tool Bar
	
	NUM_ELETYPES
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


#endif

