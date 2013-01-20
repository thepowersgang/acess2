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
#include <unicode.h>
#include <string.h>

// TODO: Include a proper keysym header
#define KEYSYM_LEFTARROW	0x50
#define KEYSYM_RIGHTARROW	0x4F

struct sTextInputInfo
{
	 int	DrawOfs;	// Byte offset for the leftmost character
	 int	CursorXOfs;	// Pixel offset of the cursor
	
	 int	CursorByteOfs;
	 int	Length;
};

// === CONSTANTS ===
const int	ciTextInput_MarginT = 3;
const int	ciTextInput_MarginB = 3;
const int	ciTextInput_MarginV = 6;	// Sum of above
const int	ciTextInput_MarginL = 3;
const int	ciTextInput_MarginR = 3;
const int	ciTextInput_MarginH = 6;

// === GLOBALS ===
tFont	*gpTextInput_Font = NULL;

// === CODE ===
void Widget_TextInput_Render(tWindow *Window, tElement *Element)
{
	struct sTextInputInfo	*info = (void*)Element->Data;
	struct sWidgetWin	*wininfo = Window->RendererInfo;
	
	// Scroll view when X offset reaches either end
	while(info->CursorXOfs >= Element->CachedW - ciTextInput_MarginH)
	{
		 int	w;
		uint32_t	cp;
		info->DrawOfs += ReadUTF8( &Element->Text[info->DrawOfs], &cp );
		WM_Render_GetTextDims(
			gpTextInput_Font,
			&Element->Text[info->DrawOfs], info->CursorByteOfs - info->DrawOfs,
			&w, NULL
			);
		info->CursorXOfs = w;
	}
	if(info->CursorXOfs < 0)
	{
		info->DrawOfs = info->CursorByteOfs;
		info->CursorXOfs = 0;
	}
	

	// Borders
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
	// - Pre-cursor
	WM_Render_DrawText(Window,
		Element->CachedX+ciTextInput_MarginL, Element->CachedY+ciTextInput_MarginT,
		Element->CachedW-ciTextInput_MarginH, Element->CachedH-ciTextInput_MarginV,
		gpTextInput_Font, TEXTINPUT_TEXT,
		&Element->Text[info->DrawOfs], -1
		);

	// Cursor
	if( wininfo->FocusedElement == Element )
	{
		WM_Render_SetTextCursor(Window,
			Element->CachedX+ciTextInput_MarginL+info->CursorXOfs,
			Element->CachedY+ciTextInput_MarginR,
			1, Element->CachedH-ciTextInput_MarginV,
			TEXTINPUT_TEXT
			);
	}
}

void Widget_TextInput_Init(tElement *Element)
{
	struct sTextInputInfo	*info;
	 int	h;

	// TODO: Select font correctly	
	WM_Render_GetTextDims(gpTextInput_Font, "jy|qJ", -1, NULL, &h);

	h += ciTextInput_MarginV;	// Border padding

	Element->MinH = h;
	Element->MinW = ciTextInput_MarginH;

	info = Element->Data = malloc(sizeof(*info));
	info->DrawOfs = 0;
	info->CursorXOfs = 0;

	// No need to explicitly update parent min dims, as the AddElement routine does that	
}

int Widget_TextInput_KeyFire(tElement *Element, int KeySym, int Character)
{
	struct sTextInputInfo	*info = Element->Data;
	 int	len;
	 int	w;
	char	*dest;
	uint32_t	cp;

//	_SysDebug("Key 0x%x fired ('%c')", Character, Character);
	
	if( Character == 0 )
	{
		switch(KeySym)
		{
		case KEYSYM_LEFTARROW:
			if( info->CursorByteOfs > 0 )
			{
				len = ReadUTF8Rev(Element->Text, info->CursorByteOfs, &cp);
				info->CursorByteOfs -= len;
				WM_Render_GetTextDims(
					gpTextInput_Font,
					Element->Text+info->CursorByteOfs,
					len, &w, 0
					);
				info->CursorXOfs -= w;
			}
			break;
		case KEYSYM_RIGHTARROW:
			if( info->CursorByteOfs < info->Length )
			{
				len = ReadUTF8(Element->Text + info->CursorByteOfs, &cp);
				WM_Render_GetTextDims(
					gpTextInput_Font,
					Element->Text+info->CursorByteOfs,
					len, &w, 0
					);
				info->CursorByteOfs += len;
				info->CursorXOfs += w;
			}
			break;
		}
		return 0;
	}

	// TODO: Don't hard code
	if(Character > 0x30000000)	return 0;

	switch(Character)
	{
	case '\t':
		return 0;
	
	case '\b':
		// Check if there is anything to delete
		if( info->CursorByteOfs == 0 )	return 0;
		// Get character to be deleted
		len = ReadUTF8Rev(Element->Text, info->CursorByteOfs, &cp);
		info->CursorByteOfs -= len;
		dest = &Element->Text[info->CursorByteOfs];
//		_SysDebug("\\b, len = %i, removing '%.*s'", len, len, dest);
		WM_Render_GetTextDims(gpTextInput_Font, dest, len, &w, 0);
		// Remove from buffer
		memmove(dest, &dest[len], info->Length - info->CursorByteOfs - len);
		info->Length -= len;
		Element->Text[info->Length] = '\0';
		// Adjust cursor
		info->CursorXOfs -= w;
		break;
	default:
		if(Character >= 0x30000000)	return 0;
		if(Character < ' ')	return 0;

		// Get required length
		len = WriteUTF8(NULL, Character);

		// Create space (possibly in the middle)	
		Element->Text = realloc(Element->Text, info->Length + len + 1);
		dest = &Element->Text[info->CursorByteOfs];
		memmove(&dest[len], dest, info->Length - info->CursorByteOfs);
		// Add the character
		WriteUTF8(dest, Character);
		info->CursorByteOfs += len;
		info->Length += len;
		Element->Text[info->Length] = '\0';

		// Update the cursor position
		// - Scrolling is implemented in render function (CachedW/CachedH are invalid atm)
		WM_Render_GetTextDims(gpTextInput_Font, dest, len, &w, NULL);
		info->CursorXOfs += w;
	}

	// TODO: Have a Widget_ function to do this instead
	WM_Invalidate(Element->Window);
	
	return 0;
}

DEFWIDGETTYPE(ELETYPE_TEXTINPUT,
	WIDGETTYPE_FLAG_NOCHILDREN,
	.Render = Widget_TextInput_Render,
	.Init = Widget_TextInput_Init,
	.KeyFire = Widget_TextInput_KeyFire
	);

