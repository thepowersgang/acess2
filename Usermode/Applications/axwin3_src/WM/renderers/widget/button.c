/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 * 
 * renderer/widget/button.c
 * - Button Widget Type
 */
#include <common.h>
#include "./common.h"
#include "./colours.h"

void Widget_Button_Render(tWindow *Window, tElement *Element)
{
	WM_Render_FillRect(
		Window,
		Element->CachedX+1, Element->CachedY+1,
		Element->CachedW-2, Element->CachedH-2,
		BUTTON_BGCOLOUR
		);
	WM_Render_DrawRect(
		Window,
		Element->CachedX, Element->CachedY,
		Element->CachedW-1, Element->CachedH-1,
		BUTTON_BORDER
		);
}

int Widget_Button_MouseButton(tElement *Element, int X, int Y, int Button, int bPress)
{
	_SysDebug("Ele %i - Button %i %s",
		Element->ID, Button,
		(bPress ? "pressed" : "released")
		);
	if(!bPress)	Widget_Fire(Element);
	return 0;	// Handled
}

DEFWIDGETTYPE(ELETYPE_BUTTON, "Button",
	0,
	.Render = Widget_Button_Render,
	.MouseButton = Widget_Button_MouseButton
	)

