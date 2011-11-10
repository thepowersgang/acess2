/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render_menu.c
 * - Pop-up menu window class/renderer
 */
#include <common.h>
#include <menu_messages.h>

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
	 int	MinLabelWidth;
	
	 int	MaxItems;
	 int	nItems;
	tMenuItem	*Items[];
} tMenuWindowInfo;

// === GLOBALS ===
tWMRenderer	gRenderer_Menu = {
	.Name = "Menu",
	.CreateWindow = Renderer_Menu_Create,
	.Redraw = Renderer_Menu_Redraw,
	.HandleMessage = Renderer_Menu_HandleMessage
};

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
	
	return ret;
}

void Renderer_Menu_Redraw(tWindow *Window)
{
	// TODO: Implement Renderer_Menu_Redraw
	
	// - Resize window to contain all items

	// - Move the window such that it is on screen
	//  > Make sure to catch if the menu can't fit fully onscreen

	// - Render each item
}

int Renderer_Menu_HandleMessage(tWindow *Window, int Msg, int Length, void *Data)
{
	switch(Msg)
	{
	case MSG_MENU_ADDITEM: {
		tMenuMsg_AddItem	*req = Data;
		tMenuItem	*item;
		if(Length < sizeof(*req) + 1)	return -1;
		if(req->Label[Length-sizeof(*req)] != '\0')	return -1;
		
		if(info->Items[req->ID])	break;
		item = malloc(sizeof(tMenuItem)+strlen(req->Label));
		info->Items[req->ID] = item;
		
		if(req->Label[0] == '\0')
		{
			// Spacer
			item->Label = NULL;
		}
		else
		{
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
			*dest = '\0';
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
				item->Label[item->KeyOffset] = '\0';
				WM_Render_GetTextDims(NULL, item->Label, &item->UnderlineX, NULL);
				item->Label[item->KeyOffset] = tmp;
				tmp = item->Label[item->KeyOffset+1];
				item->Label[item->KeyOffset+1] = '\0';
				WM_Render_GetTextDims(NULL, item->Label+item->KeyOffset, &item->UnderlineW, NULL);
				item->Label[item->KeyOffset+1] = tmp;
			}
			// - Labels
			WM_Render_GetTextDims(NULL, item->Label, &item->LabelWidth, NULL);
			if(item->Shortcut)
				WM_Render_GetTextDims(NULL, item->Shortcut, &item->ShortcutWidth, NULL);
			else
				item->ShortcutWidth = 0;
		}
	
		break; }
	
	// Only message to pass to client
	case MSG_MENU_SELECT:
		return 1;
	}
	return 0;
}

