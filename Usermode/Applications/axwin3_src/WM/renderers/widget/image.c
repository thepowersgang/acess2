/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 * 
 * renderer/widget/image.c
 * - Image Widget Type
 */
#include <common.h>
#include "./common.h"
#include "./colours.h"
#include "../../include/image.h"

void Widget_Image_Render(tWindow *Window, tElement *Element)
{
	WM_Render_DrawImage(
		Window,
		Element->CachedX, Element->CachedY,
		Element->CachedW, Element->CachedH,
		Element->Data
		);
}

void Widget_Image_UpdateText(tElement *Element, const char *Text)
{
	if(Element->Data)	free(Element->Data);
	Element->Data = Image_Load( Text );
	if(!Element->Data) {
//		Element->Flags &= ~ELEFLAG_FIXEDSIZE;
		return ;
	}
	
	Element->CachedW = ((tImage*)Element->Data)->Width;
	Element->CachedH = ((tImage*)Element->Data)->Height;

	Element->MinW = ((tImage*)Element->Data)->Width;
	Element->MinH = ((tImage*)Element->Data)->Width;
	
	Widget_UpdateMinDims(Element->Parent);
	
	// NOTE: Doesn't update Element->Text because it's not really needed here
}

DEFWIDGETTYPE(ELETYPE_IMAGE,
	WIDGETTYPE_FLAG_NOCHILDREN,
	.Render = Widget_Image_Render,
	.UpdateText = Widget_Image_UpdateText
	);

