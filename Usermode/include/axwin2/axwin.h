/**
 * \file axwin2/axwin.h
 * \author John Hodge (thePowersGang)
 * \brief AxWin Core functions
 */
#ifndef _AXWIN2_AXWIN_H
#define _AXWIN2_AXWIN_H

#include <stdlib.h>
#include <stdint.h>

#include <axwin2/messages.h>

// === Core Types ===
typedef struct sAxWin_Element	tAxWin_Element;
//typedef struct sAxWin_Message	tAxWin_Message;
typedef int	tAxWin_MessageCallback(tAxWin_Message *);
//typedef int	tAxWin_MessageCallback(void *Source, int Message, int Length, void *Data);

// === Functions ===
extern int	AxWin_Register(const char *ApplicationName, tAxWin_MessageCallback *DefaultHandler);
extern tAxWin_Element	*AxWin_CreateTab(const char *TabTitle);
extern tAxWin_Element	*AxWin_AddMenuItem(tAxWin_Element *Parent, const char *Label, int Message);

extern int	AxWin_MessageLoop(void);
extern int	AxWin_SendMessage(tAxWin_Message *Message);
extern tAxWin_Message	*AxWin_WaitForMessage(void);
extern int	AxWin_HandleMessage(tAxWin_Message *Message);

// === Window Control ===

extern tAxWin_Element	*AxWin_CreateElement(tAxWin_Element *Parent, int ElementType, int Flags, const char *DebugName);
extern void	AxWin_SetFlags(tAxWin_Element *Element, int Flags);
extern void	AxWin_SetText(tAxWin_Element *Element, const char *Text);
extern void	AxWin_SetSize(tAxWin_Element *Element, int Size);
extern void	AxWin_DeleteElement(tAxWin_Element *Element);

// === CONSTANTS ===
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

/**
 */
enum eElementTypes
{
	ELETYPE_NONE,

	ELETYPE_WINDOW,
	
	ELETYPE_BOX,	//!< Content box (invisible in itself)
	ELETYPE_TABBAR,	//!< Tab Bar
	ELETYPE_TOOLBAR,	//!< Tool Bar
	
	ELETYPE_BUTTON,	//!< Push Button
	
	ELETYPE_TEXT,	//!< Text
	ELETYPE_IMAGE,	//!< Image
	
	ELETYPE_SPACER,	//!< Visual Spacer (horizontal / vertical rule)
	
	MAX_ELETYPES	= 0x100
};

#endif
