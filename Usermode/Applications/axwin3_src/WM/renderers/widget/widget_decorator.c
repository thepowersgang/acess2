/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 * 
 * renderer_widget_decorator.c
 * - Widget Decorator
 */
#include <common.h>
#include <wm.h>
#include <renderer_widget.h>

#define BORDER_EVERYTHING	1

#include "./colours.h"

// === CODE ===
void Widget_Decorator_RenderWidget(tWindow *Window, tElement *Element)
{
//	_SysDebug("Widget_Decorator_RenderWidget: (Element={Type:%i,(%i,%i) %ix%i})",
//		Element->Type,
//		Element->CachedX, Element->CachedY,
//		Element->CachedW, Element->CachedH
//		);
	
	#if BORDER_EVERYTHING
	WM_Render_DrawRect(
		Window,
		Element->CachedX, Element->CachedY,
		Element->CachedW, Element->CachedH,
		0
		);
	#endif
	
	switch(Element->Type)
	{
	case ELETYPE_NONE:
	case ELETYPE_BOX:	break;	// Box is a meta-element
	
	case ELETYPE_TABBAR:	// Tab Bar
		WM_Render_DrawRect(
			Window,
			Element->CachedX, Element->CachedY,
			Element->CachedW, Element->CachedH,
			BOX_BORDER
			);
		WM_Render_FillRect(
			Window,
			Element->CachedX+1, Element->CachedY+1,
			Element->CachedW-2, Element->CachedH-2,
			BOX_BGCOLOUR
			);
		// Enumerate Items.
		break;
	case ELETYPE_TOOLBAR:	// Tool Bar
		WM_Render_DrawRect(
			Window,
			Element->CachedX, Element->CachedY,
			Element->CachedW, Element->CachedH,
			BOX_BORDER
			);
		WM_Render_FillRect(
			Window,
			Element->CachedX+1, Element->CachedY+1,
			Element->CachedW-2, Element->CachedH-2,
			BOX_BGCOLOUR
			);
		break;
	
	case ELETYPE_SPACER:	// Spacer (subtle line)
		WM_Render_FillRect(
			Window,
			Element->CachedX+3, Element->CachedY+3,
			Element->CachedW-6, Element->CachedH-6,
			BOX_BORDER
			);
		break;
	
	case ELETYPE_BUTTON:	// Button
		break;

	// Text input field / Text Box
	case ELETYPE_TEXTINPUT:
	case ELETYPE_TEXTBOX:
		break;
	
	case ELETYPE_TEXT:
		break;
	
	case ELETYPE_IMAGE:
		break;
		
	default:
		_SysDebug(" ERROR: Unknown type %i", Element->Type);
		break;
	}
}
