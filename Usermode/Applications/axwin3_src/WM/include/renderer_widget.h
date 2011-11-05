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
#include <axwin3/widget.h>

#include <widget_messages.h>

// === TYPES ===
typedef struct sWidgetWin	tWidgetWin;
typedef struct sAxWin_Element	tElement;

// === STRUCTURES ===
struct sAxWin_Element
{
	enum eElementTypes	Type;

	uint32_t	ID;	//!< Application provided ID number
	tElement	*ListNext;	//!< Next element in bucket
	
	// Element Tree
	tElement	*Parent;
	tElement	*FirstChild;
	tElement	*LastChild;
	tElement	*NextSibling;
	
	// User modifiable attributes	
	short	PaddingL, PaddingR;
	short	PaddingT, PaddingB;
	short	GapSize;
	
	uint32_t	Flags;
	
	short	FixedWith;	//!< Fixed lengthways Size attribute (height)
	short	FixedCross;	//!< Fixed Cross Size attribute (width)
	
	tColour	BackgroundColour;

	char	*Text;
	
	// -- Attributes maitained by the element code
	// Not touched by the user
	short	MinWith;	//!< Minimum long size
	short	MinCross;	//!< Minimum cross size
	void	*Data;	//!< Per-type data
	
	// -- Render Cache
	short	CachedX, CachedY;
	short	CachedW, CachedH;
	
	char	DebugName[];
};
struct sWidgetWin
{
	tElement	RootElement;
	
	 int	TableSize;	//!< Number of entries, anything over will wrap
	tElement	*ElementTable[];	//!< Hash table essentially
};

// === FUNCTIONS === 
extern void	Widget_Decorator_RenderWidget(tWindow *Window, tElement *Element);

#endif

