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

	tWindow 	*Window;
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
	short	MinW;	//!< Minimum long size
	short	MinH;	//!< Minimum cross size
	void	*Data;	//!< Per-type data
	
	// -- Render Cache
	short	CachedX, CachedY;
	short	CachedW, CachedH;
};
struct sWidgetWin
{
	tElement	RootElement;

	tElement	*FocusedElement;
	
	 int	TableSize;	//!< Number of entries, anything over will wrap
	tElement	*ElementTable[];	//!< Hash table essentially
};

#endif

