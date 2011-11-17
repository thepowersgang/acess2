/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 * 
 * renderer/widget/textinput.c
 * - Single line text box
 */
#include <common.h>
#include "./common.h"
#include "./colours.h"

void Widget_TextInput_Render(tWindow *Window, tElement *Element)
{
	WM_Render_FillRect(
		Window, 
		Element->CachedX, Element->CachedY,
		Element->CachedW, Element->CachedH,
		TEXTINPUT_BACKGROUND
		);
	WM_Render_DrawRect(
		Window, 
		Element->CachedX, Element->CachedY,
		Element->CachedW, Element->CachedH,
		TEXTINPUT_BORDER_OUT
		);
	WM_Render_DrawRect(
		Window, 
		Element->CachedX+1, Element->CachedY+1,
		Element->CachedW-2, Element->CachedH-2,
		TEXTINPUT_BORDER_IN
		);
	// TODO: Cursor?
}

void Widget_TextInput_Init(tElement *Element)
{
	 int	h;

	// TODO: Select font correctly	
	WM_Render_GetTextDims(NULL, "jJ", NULL, &h);
	
	if( Element->Parent && (Element->Parent->Flags & ELEFLAG_VERTICAL) )
		Element->MinWith = h;
	else
		Element->MinCross = h;

	// No need to explicitly update parent min dims, as the AddElement routine does that	
}

DEFWIDGETTYPE(ELETYPE_TEXTINPUT,
	.Render = Widget_TextInput_Render,
	.Init = Widget_TextInput_Init
	);


