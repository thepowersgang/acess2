/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 * 
 * renderer/widget/spacer.
 * - Horizontal / Verical Line
 */
#include <common.h>
#include "./common.h"
#include "./colours.h"

#define SPACER_RULE_SIZE	3
#define SPACER_RULE_OFFSET	1

void Widget_Spacer_Render(tWindow *Window, tElement *Element)
{
//	_SysDebug("Spacer at (%i,%i) %ix%i",
//		Element->CachedX, Element->CachedY,
//		Element->CachedW, Element->CachedH
//		);
	if( Element->Parent && (Element->Parent->Flags & ELEFLAG_VERTICAL) )
	{
		WM_Render_DrawRect(
			Window,
			Element->CachedX+1, Element->CachedY+1,
			Element->CachedW-2, SPACER_RULE_SIZE,
			BOX_BORDER
			);
	}
	else
	{
		WM_Render_DrawRect(
			Window,
			Element->CachedX+1, Element->CachedY+1,
			SPACER_RULE_SIZE, Element->CachedH-2,
			BOX_BORDER
			);
	}
}

void Widget_Spacer_Init(tElement *Element)
{
	Element->MinH = SPACER_RULE_SIZE+2;
	Element->MinW = SPACER_RULE_SIZE+2;
}

DEFWIDGETTYPE(ELETYPE_SPACER,
	WIDGETTYPE_FLAG_NOCHILDREN,
	.Render = Widget_Spacer_Render,
	.Init = Widget_Spacer_Init
	);


