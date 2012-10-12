/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 * 
 * renderer/widget/subwin.c
 * - Embedded Window Widget Type
 */
#include <common.h>
#include "./common.h"
#include "./colours.h"

void Widget_SubWin_Render(tWindow *Window, tElement *Element)
{
	// Note: Doesn't actually render, but does poke the child window
	WM_MoveWindow(Element->Data, Element->CachedX, Element->CachedY);
	WM_ResizeWindow(Element->Data, Element->CachedW, Element->CachedH);
}

DEFWIDGETTYPE(ELETYPE_SUBWIN,
	WIDGETTYPE_FLAG_NOCHILDREN,
	.Render = Widget_SubWin_Render
	)

