/*
 * AxWin3 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Entrypoint and setup
 */
#include <axwin3/axwin.h>
#include <axwin3/widget.h>
#include "include/internal.h"
#include <stdlib.h>
#include <widget_messages.h>
#include <string.h>

// === STRUCTURES ===
struct sAxWin3_Widget
{
	tHWND	Window;
	uint32_t	ID;
	tAxWin3_Widget_Callback	Callback;
};

typedef struct
{
	 int	nElements;
	tAxWin3_Widget	**Elements;
	 int	FirstFreeID;
	tAxWin3_Widget	RootElement;
	// Callbacks for each element
} tWidgetWindowInfo;

// === CODE ===
int AxWin3_Widget_MessageHandler(tHWND Window, int Size, void *Data)
{
	return 0;
}

tHWND AxWin3_Widget_CreateWindow(tHWND Parent, int W, int H, int RootEleFlags)
{
	tHWND	ret;
	tWidgetWindowInfo	*info;
	
	ret = AxWin3_CreateWindow(
		Parent, "Widget", RootEleFlags,
		sizeof(*info) + sizeof(tAxWin3_Widget), AxWin3_Widget_MessageHandler
		);
	info = AxWin3_int_GetDataPtr(ret);

	info->RootElement.Window = ret;
	info->RootElement.ID = -1;

	AxWin3_ResizeWindow(ret, W, H);
	
	return ret;
}

void AxWin3_Widget_DestroyWindow(tHWND Window)
{
	// Free all element structures
	
	// Free array
	
	// Request window to be destroyed (will clean up any data stored in tHWND)
}

tAxWin3_Widget *AxWin3_Widget_GetRoot(tHWND Window)
{
	tWidgetWindowInfo	*info = AxWin3_int_GetDataPtr(Window);
	return &info->RootElement;
}

tAxWin3_Widget *AxWin3_Widget_AddWidget(tAxWin3_Widget *Parent, int Type, int Flags, const char *DebugName)
{
	 int	newID;
	tWidgetWindowInfo	*info;
	tAxWin3_Widget	*ret;

	if(!Parent)	return NULL;

	info = AxWin3_int_GetDataPtr(Parent->Window);
	
	// Assign ID
	// TODO: Atomicity
	for( newID = info->FirstFreeID; newID < info->nElements; newID ++ )
	{
		if( info->Elements[newID] == NULL )
			break;
	}
	if( info->nElements == 0 || info->Elements[newID] )
	{
		info->nElements ++;
		info->Elements = realloc(info->Elements, sizeof(*info->Elements)*info->nElements);
		newID = info->nElements - 1;
	}
	info->Elements[newID] = (void*)-1;
	
	// Create new widget structure
	ret = malloc(sizeof(tAxWin3_Widget));
	ret->Window = Parent->Window;
	ret->ID = newID;
	ret->Callback = NULL;

	info->Elements[newID] = ret;

	// Send create widget message
	{
		char	tmp[sizeof(tWidgetMsg_Create)+1];
		tWidgetMsg_Create	*msg = (void*)tmp;
		msg->Parent = Parent->ID;
		msg->NewID = newID;
		msg->DebugName[0] = '\0';
		AxWin3_SendMessage(ret->Window, ret->Window, MSG_WIDGET_CREATE, sizeof(tmp), tmp);
	}

	return ret;
}

void AxWin3_Widget_DelWidget(tAxWin3_Widget *Widget)
{
	tWidgetMsg_Delete	msg;
	tWidgetWindowInfo	*info = AxWin3_int_GetDataPtr(Widget->Window);
	
	msg.WidgetID = Widget->ID;
	AxWin3_SendMessage(Widget->Window, Widget->Window, MSG_WIDGET_DELETE, sizeof(msg), &msg);
	
	info->Elements[Widget->ID] = NULL;
	if(Widget->ID < info->FirstFreeID)
		info->FirstFreeID = Widget->ID;
	free(Widget);
}

void AxWin3_Widget_SetSize(tAxWin3_Widget *Widget, int Size)
{
	tWidgetMsg_SetSize	msg;
	
	msg.WidgetID = Widget->ID;
	msg.Value = Size;
	AxWin3_SendMessage(Widget->Window, Widget->Window, MSG_WIDGET_SETSIZE, sizeof(msg), &msg);
}

void AxWin3_Widget_SetText(tAxWin3_Widget *Widget, const char *Text)
{
	char	buf[sizeof(tWidgetMsg_SetText) + strlen(Text) + 1];
	tWidgetMsg_SetText	*msg = (void*)buf;
	
	msg->WidgetID = Widget->ID;
	strcpy(msg->Text, Text);
	
	AxWin3_SendMessage(Widget->Window, Widget->Window, MSG_WIDGET_SETTEXT, sizeof(buf), buf);
}

void AxWin3_Widget_SetColour(tAxWin3_Widget *Widget, int Index, tAxWin3_Colour Colour)
{
	tWidgetMsg_SetColour	msg;
	
	msg.WidgetID = Widget->ID;
	msg.Index = Index;
	msg.Colour = Colour;
	AxWin3_SendMessage(Widget->Window, Widget->Window, MSG_WIDGET_SETCOLOUR, sizeof(msg), &msg);
}
