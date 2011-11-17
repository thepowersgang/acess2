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
	
	if(Element->Parent && (Element->Parent->Flags & ELEFLAG_VERTICAL) ) {
		Element->MinCross = ((tImage*)Element->Data)->Width;
		Element->MinWith = ((tImage*)Element->Data)->Height;
	}
	else {
		Element->MinWith = ((tImage*)Element->Data)->Width;
		Element->MinCross = ((tImage*)Element->Data)->Height;
	}

	Widget_UpdateMinDims(Element->Parent);
	
	// NOTE: Doesn't update Element->Text because it's useless
}

DEFWIDGETTYPE(ELETYPE_IMAGE,
	.Render = Widget_Image_Render,
	.UpdateText = Widget_Image_UpdateText
	);

