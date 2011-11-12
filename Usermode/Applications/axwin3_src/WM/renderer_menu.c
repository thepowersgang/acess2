/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render_menu.c
 * - Pop-up menu window class/renderer
 */
#include <common.h>
#include <wm_renderer.h>
#include <menu_messages.h>
#include <wm_messages.h>
#include <stdlib.h>
#include <string.h>

// === STRUCTURES ===
typedef struct sMenuItem
{
	// Settings
	char	*Label;
	char	*Shortcut;
	 int	KeyOffset;
	 int	Flags;
	
	// Cached values
	 int	LabelWidth;	
	 int	ShortcutWidth;
	 int	UnderlineX;
	 int	UnderlineW;

	char	Data[];
} tMenuItem;

typedef struct sMenuWindowInfo
{
	 int	MaxLabelWidth;
	 int	MaxShortcutWidth;
	 int	CachedW;

	 int	HilightedItem;	

	 int	MaxItems;
	 int	nItems;
	tMenuItem	*Items[];
} tMenuWindowInfo;

// === PROTOTYPES ===
void	Renderer_Menu_Init(void);
tWindow	*Renderer_Menu_Create(int Argument);
void	Renderer_Menu_Redraw(tWindow *Window);
 int	Renderer_Menu_HandleMessage(tWindow *Window, int Msg, int Length, void *Data);

// === CONSTANTS ===
const int	ciMenu_Gap = 10; 	// Gap between label and shortcut
const int	ciMenu_TopPadding = 2;
const int	ciMenu_BottomPadding = 2;
const int	ciMenu_LeftPadding = 2;
const int	ciMenu_RightPadding = 2;
const int	ciMenu_FontHeight = 16;
const int	ciMenu_ItemHeight = 20;
const int	ciMenu_SpacerHeight = 5;
const tColour	cMenu_BackgroundColour = 0xCCCCCC;
const tColour	cMenu_BorderColour   = 0x000000;
const tColour	cMenu_SpacerColour   = 0x404040;
const tColour	cMenu_LabelColour    = 0x000000;
const tColour	cMenu_ShortcutColour = 0x404040;
const tColour	cMenu_HilightColour  = 0xE0E0E0;

// === GLOBALS ===
tWMRenderer	gRenderer_Menu = {
	.Name = "Menu",
	.CreateWindow = Renderer_Menu_Create,
	.Redraw = Renderer_Menu_Redraw,
	.HandleMessage = Renderer_Menu_HandleMessage
};
tFont	*gMenu_Font = NULL;	// System monospace

// === CODE ===
void Renderer_Menu_Init(void)
{
	WM_RegisterRenderer(&gRenderer_Menu);
}

tWindow	*Renderer_Menu_Create(int Argument)
{
	tWindow	*ret;
	tMenuWindowInfo	*info;

	if(Argument < 5)	Argument = 5;
	if(Argument > 200)	Argument = 200;	

	ret = WM_CreateWindowStruct(sizeof(*info) + Argument*sizeof(info->Items[0]));
	info = ret->RendererInfo;
	info->MaxItems = Argument;
	info->HilightedItem = -1;

	_SysDebug("Renderer_Menu_Create: ->MaxItems = %i", info->MaxItems);
	
	return ret;
}

void Renderer_Menu_Redraw(tWindow *Window)
{
	tMenuWindowInfo	*info = Window->RendererInfo;
	 int	w, h, y, i;

	w = info->CachedW;
	h = ciMenu_TopPadding + ciMenu_BottomPadding;
	for( i = 0; i < info->nItems; i ++ )
	{
		if( !info->Items[i] )	continue;
		
		if(info->Items[i]->Label)
			h += ciMenu_ItemHeight;
		else
			h += ciMenu_SpacerHeight;
	}

//	_SysDebug("w = %i, h = %i", w, h);

	// - Resize window to contain all items
	WM_ResizeWindow(Window, w, h);

	// - Move the window such that it is on screen
	//  > Make sure to catch if the menu can't fit fully onscreen

	// - Clear
	WM_Render_FillRect(Window, 0, 0, w, h, cMenu_BackgroundColour);
	WM_Render_DrawRect(Window, 0, 0, w, h, cMenu_BorderColour);

	// - Render each item
	y = ciMenu_TopPadding;
	for( i = 0; i < info->nItems; i ++ )
	{
		tMenuItem	*item = info->Items[i];
		
		// Unused slot
		if(!item)	continue;
		
		// Spacer
		if(!item->Label)
		{
			WM_Render_FillRect(Window,
				1, y + ciMenu_SpacerHeight/2,
				w-2, 1,
				cMenu_SpacerColour
				);
			y += ciMenu_SpacerHeight;
			continue ;
		}
	
		// Hilight
		if( info->HilightedItem == i )
		{
			WM_Render_FillRect(Window,
				1, y,
				w-2, ciMenu_ItemHeight,
				cMenu_HilightColour
				);
		}
	
		// Text
		WM_Render_DrawText(Window,
			ciMenu_LeftPadding, y,
			w, ciMenu_ItemHeight,
			gMenu_Font,
			cMenu_LabelColour,
			item->Label
			);
		// Underline
		if(item->UnderlineW)
		{
			WM_Render_FillRect(Window,
				ciMenu_LeftPadding + item->UnderlineX, y + ciMenu_FontHeight + 1,
				item->UnderlineW, 1,
				cMenu_LabelColour
				);
		}
		
		// Shortcut key
		if(item->Shortcut)
		{
			WM_Render_DrawText(Window,
				w - item->ShortcutWidth - ciMenu_RightPadding, y,
				w, ciMenu_ItemHeight,
				gMenu_Font,
				cMenu_ShortcutColour,
				item->Shortcut
				);
		}
		
		y += ciMenu_ItemHeight;
	}
}

int Renderer_Menu_int_AddItem(tWindow *Window, int Length, void *Data)
{
	tMenuWindowInfo	*info = Window->RendererInfo;
	tMenuMsg_AddItem	*req = Data;
	tMenuItem	*item;
	
	// Sanity checking
	// - Message length
	if(Length < sizeof(*req) + 1 || req->Label[Length-sizeof(*req)-1] != '\0') {
		_SysDebug("Renderer_Menu_int_AddItem: Size checks failed");
		return -1;
	}
	// - ID Number
	if(req->ID >= info->MaxItems) {
		_SysDebug("Renderer_Menu_int_AddItem: ID (%i) >= MaxItems (%i)",
			req->ID, info->MaxItems);
		return -1;
	}
	
	// Don't overwrite
	if(info->Items[req->ID])	return 0;
	// Bookkeeping
	if(req->ID >= info->nItems)	info->nItems = req->ID + 1;
	// Allocate
	item = malloc(sizeof(tMenuItem)+strlen(req->Label));
	info->Items[req->ID] = item;
	
	if(req->Label[0] == '\0')
	{
		// Spacer
		item->Label = NULL;
		
		return 0;
	}
	
	// Actual item
	char	*dest = item->Data;
	char	*src = req->Label;
	 int	ofs = 0;

	// - Main label
	item->KeyOffset = -1;
	item->Label = dest;
	for(ofs = 0; *src && *src != '\t'; ofs ++)
	{
		if(*src == '&') {
			*dest = '\0';
			item->KeyOffset = ofs;
			src ++;
		}
		else {
			*dest++ = *src++;
		}
	}
	*dest++ = '\0';
	// - Key combo / Shortcut
	if(*src)
	{
		src ++;
		item->Shortcut = dest;
		strcpy(item->Shortcut, src);
	}
	else
	{
		item->Shortcut = NULL;
	}
	
	// Get dimensions
	// - Underline (hotkey)
	if(item->KeyOffset == -1)
	{
		item->UnderlineX = 0;
		item->UnderlineW = 0;
	}
	else
	{
		char	tmp = item->Label[item->KeyOffset];
		// Get width of preceding substring
		item->Label[item->KeyOffset] = '\0';
		WM_Render_GetTextDims(NULL, item->Label, &item->UnderlineX, NULL);
		// Get the width of the underlined character
		// TODO: Fix for high UTF-8 characters
		item->Label[item->KeyOffset] = tmp;
		tmp = item->Label[item->KeyOffset+1];
		item->Label[item->KeyOffset+1] = '\0';
		WM_Render_GetTextDims(
			NULL, item->Label+item->KeyOffset,
			&item->UnderlineW, NULL
			);
		item->Label[item->KeyOffset+1] = tmp;
	}
	// - Labels
	WM_Render_GetTextDims(NULL, item->Label, &item->LabelWidth, NULL);
	if(item->Shortcut)
		WM_Render_GetTextDims(NULL, item->Shortcut, &item->ShortcutWidth, NULL);
	else
		item->ShortcutWidth = 0;
	
	if( item->LabelWidth > info->MaxLabelWidth )
		info->MaxLabelWidth = item->LabelWidth;
	if( item->ShortcutWidth > info->MaxShortcutWidth )
		info->MaxShortcutWidth = item->ShortcutWidth;
	
	if( info->MaxLabelWidth + info->MaxShortcutWidth + ciMenu_Gap > info->CachedW )
	{
		info->CachedW = ciMenu_LeftPadding + info->MaxLabelWidth
			+ ciMenu_Gap + info->MaxShortcutWidth
			+ ciMenu_RightPadding;
		// TODO: Smarter height?
		WM_ResizeWindow(Window, info->CachedW, info->nItems*ciMenu_ItemHeight);
	}
	
	return 0;
}

int Renderer_Menu_int_GetItemByPos(tWindow *Window, tMenuWindowInfo *Info, int X, int Y)
{
	 int	i;

	if( X < 0 || X >= Window->W )
		return -1;
	
	for( i = 0; i < Info->nItems; i ++ )
	{
		if( !Info->Items[i] )	continue;
			
		if( !Info->Items[i]->Label )
		{
			// Spacer - doesn't hilight
			if(Y < ciMenu_SpacerHeight) {
				return -1;
			}
			Y -= ciMenu_SpacerHeight;
		}
		else
		{
			// Normal item, set the hilight
			if(Y < ciMenu_ItemHeight) {
				return i;
			}
			Y -= ciMenu_ItemHeight;
		}
	}
	return -1;
}

int Renderer_Menu_HandleMessage(tWindow *Window, int Msg, int Length, void *Data)
{
	tMenuWindowInfo	*info = Window->RendererInfo;
	switch(Msg)
	{
	case WNDMSG_SHOW: {
		struct sWndMsg_Bool	*msg = Data;
		if(Length < sizeof(*msg))	return -1;
		if(msg->Val) {
			// Take focus?
			_SysDebug(" - Shown, take focus");
			WM_GiveFocus(Window);
		}
		else
		{
			// Hide Children
			_SysDebug("- Hidden, hide the children!");
		}
		return 0; }
	case WNDMSG_FOCUS: {
		struct sWndMsg_Bool	*msg = Data;
		if(Length < sizeof(*msg))	return -1;
		if(!msg->Val) {
			// TODO: Catch if focus was given away to a child
			_SysDebug("- Lost focus");
			WM_ShowWindow(Window, 0);	// Hide!
		}
 		else {
			_SysDebug("- Focus gained, TODO: Show accel keys");
		}
		return 0; }

	case WNDMSG_MOUSEBTN: {
		struct sWndMsg_MouseButton	*msg = Data;
		 int	item;
		
		if(Length < sizeof(*msg))	return -1;

		if(msg->Button == 0 && msg->bPressed == 0)
		{
			item = Renderer_Menu_int_GetItemByPos(Window, info, msg->X, msg->Y);
			if(item != -1)
			{
				tMenuMsg_Select	_msg;
				// TODO: Ignore sub-menus too
				_msg.ID = item;
				WM_SendMessage(Window, Window, MSG_MENU_SELECT, sizeof(_msg), &_msg);
				WM_ShowWindow(Window, 0);
			}
		}
				

		return 0; }	

	case WNDMSG_MOUSEMOVE: {
		struct sWndMsg_MouseMove	*msg = Data;
		 int	new_hilight;

		if(Length < sizeof(*msg))	return -1;

		new_hilight = Renderer_Menu_int_GetItemByPos(Window, info, msg->X, msg->Y);

		if( new_hilight != info->HilightedItem )
		{
			info->HilightedItem = new_hilight;
			// TODO: Change sub-menu
			WM_Invalidate(Window);
		}

		return 0; }

	// Manipulation messages
	case MSG_MENU_ADDITEM:
		_SysDebug("MSG_MENU_ADDITEM");
		return Renderer_Menu_int_AddItem(Window, Length, Data);
	
	// Only message to pass to client
	case MSG_MENU_SELECT:
		return 1;
	}
	return 0;
}

