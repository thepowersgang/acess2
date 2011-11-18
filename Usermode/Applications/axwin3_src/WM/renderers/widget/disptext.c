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
#include <string.h>

void Widget_DispText_Render(tWindow *Window, tElement *Element)
{
	WM_Render_DrawText(
		Window,
		Element->CachedX+1, Element->CachedY+1,
		Element->CachedW-2, Element->CachedH-2,
		NULL, TEXT_COLOUR,
		Element->Text
		);
}

void Widget_DispText_UpdateText(tElement *Element, const char *Text)
{
	 int	w=0, h=0;

	if(Element->Text)	free(Element->Text);
	Element->Text = strdup(Text);

	WM_Render_GetTextDims(NULL, Element->Text, &w, &h);

	// Apply edge padding
	w += 2;	h += 2;
	
	if(Element->Parent && (Element->Parent->Flags & ELEFLAG_VERTICAL)) {
		Element->MinCross = w;
		Element->MinWith = h;
	}
	else {
		Element->MinWith = w;
		Element->MinCross = h;
	}

	Widget_UpdateMinDims(Element->Parent);
}

DEFWIDGETTYPE(ELETYPE_TEXT,
	WIDGETTYPE_FLAG_NOCHILDREN,
	.Render = Widget_DispText_Render,
	.UpdateText = Widget_DispText_UpdateText
	);

