/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 * 
 * renderer/widget/textinput.c
 * - Single line text box
 *
 * TODO: Support Right-to-Left text
 */
#include <common.h>
#include "./common.h"
#include "./colours.h"

struct sTextInputInfo
{
	 int	DrawOfs;	// Byte offset for the leftmost character
	 int	CursorXOfs;	// Pixel offset of the cursor
};

void Widget_TextInput_Render(tWindow *Window, tElement *Element)
{
	struct sTextInputInfo	*info = (void*)Element->Data;
	struct sWidgetWin	*wininfo = Window->RendererInfo;
	
	WM_Render_FillRect(Window, 
		Element->CachedX, Element->CachedY,
		Element->CachedW, Element->CachedH,
		TEXTINPUT_BACKGROUND
		);
	WM_Render_DrawRect(Window, 
		Element->CachedX, Element->CachedY,
		Element->CachedW, Element->CachedH,
		TEXTINPUT_BORDER_OUT
		);
	WM_Render_DrawRect(Window, 
		Element->CachedX+1, Element->CachedY+1,
		Element->CachedW-2, Element->CachedH-2,
		TEXTINPUT_BORDER_IN
		);
	
	// Text
	WM_Render_DrawText(Window,
		Element->CachedX+2, Element->CachedY+2,
		Element->CachedW-4, Element->CachedW-4,
		NULL, TEXTINPUT_TEXT,
		&Element->Text[info->DrawOfs]
		);

	// TODO: Determine if this element has focus
	if( wininfo->FocusedElement == Element )
	{
		// TODO: Multiple Cursors
		WM_Render_SetTextCursor(Window,
			Element->CachedX+2+info->CursorXOfs,
			Element->CachedY+2,
			1, Element->CachedH-4,
			TEXTINPUT_TEXT
			);
	}
}

void Widget_TextInput_Init(tElement *Element)
{
	struct sTextInputInfo	*info;
	 int	h;

	// TODO: Select font correctly	
	WM_Render_GetTextDims(NULL, "jJ", NULL, &h);

	h += 2+2;	// Border padding	

	Element->MinH = h;
	Element->MinW = 4;

	info = Element->Data = malloc(sizeof(*info));
	info->DrawOfs = 0;
	info->CursorXOfs = 0;

	// No need to explicitly update parent min dims, as the AddElement routine does that	
}

int Widget_TextInput_KeyFire(tElement *Ele, int KeySym, int Character)
{
	_SysDebug("Key 0x%x fired ('%c')", Character, Character);
	return 0;
}

DEFWIDGETTYPE(ELETYPE_TEXTINPUT,
	WIDGETTYPE_FLAG_NOCHILDREN,
	.Render = Widget_TextInput_Render,
	.Init = Widget_TextInput_Init,
	.KeyFire = Widget_TextInput_KeyFire
	);

