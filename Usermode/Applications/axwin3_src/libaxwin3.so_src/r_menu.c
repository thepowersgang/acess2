/*
 * AxWin3 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * r_menu.c
 * - Menu renderer interface
 */
#include <stdlib.h>
#include <axwin3/axwin.h>
#include <axwin3/menu.h>
#include <menu_messages.h>
#include "include/internal.h"
#include <string.h>

// === TYPES ===
typedef struct sMenuWindowInfo	tMenuWindowInfo;

struct sAxWin3_MenuItem
{
	tHWND	Window;
	 int	ID;
	tAxWin3_Menu_Callback	Callback;
	void	*CbPtr;
	tHWND	SubMenu;
};

struct sMenuWindowInfo
{
	 int	nItems;
	tAxWin3_MenuItem	*Items;
};

// === CODE ===
int AxWin3_Menu_int_Callback(tHWND Window, int Message, int Length, void *Data)
{
	tMenuWindowInfo	*info = AxWin3_int_GetDataPtr(Window);
	switch(Message)
	{
	case MSG_MENU_SELECT: {
		tMenuMsg_Select	*msg = Data;
		tAxWin3_MenuItem	*item;
		if(Length < sizeof(tMenuMsg_Select))	return -1;
		if(msg->ID >= info->nItems)	return -1;
		item = &info->Items[msg->ID];
		if(item->Callback)	item->Callback(item->CbPtr);
		return 0; }
	}
	return 1;
}

tHWND AxWin3_Menu_Create(tHWND Parent)
{
	tHWND	ret;
	tMenuWindowInfo	*info;
	
	ret = AxWin3_CreateWindow(Parent, "Menu", 0, sizeof(*info), AxWin3_Menu_int_Callback);
	if(!ret)	return ret;
	
	info = AxWin3_int_GetDataPtr(ret);
	info->nItems = 0;
	info->Items = NULL;
	
	return ret;
}

void AxWin3_Menu_ShowAt(tHWND Menu, int X, int Y)
{
	AxWin3_MoveWindow(Menu, X, Y);
	AxWin3_ShowWindow(Menu, 1);
	AxWin3_FocusWindow(Menu);
}

tAxWin3_MenuItem *AxWin3_Menu_AddItem(
	tHWND Menu, const char *Label,
	tAxWin3_Menu_Callback Cb, void *Ptr,
	int Flags, tHWND SubMenu
	)
{
	tMenuWindowInfo	*info;
	tAxWin3_MenuItem	*ret;
	
	info = AxWin3_int_GetDataPtr(Menu);
	
	info->nItems ++;
	info->Items = realloc(info->Items, sizeof(*info->Items)*info->nItems);
	if(!info->Items) {
		_SysDebug("ERROR: Realloc Failed");
		return NULL;
	}
	
	ret = &info->Items[info->nItems-1];
	ret->ID = info->nItems - 1;
	ret->Window = Menu;
	ret->Callback = Cb;
	ret->CbPtr = Ptr;
	ret->SubMenu = SubMenu;	

	{
		tMenuIPC_AddItem	*req;
		 int	data_size;
		if(!Label)	Label = "";
		data_size = sizeof(*req)+strlen(Label)+1;
		req = malloc(data_size);
		req->ID = ret->ID;
		req->Flags = Flags;
		req->SubMenuID = AxWin3_int_GetWindowID(SubMenu);
		strcpy(req->Label, Label);
		AxWin3_SendIPC(Menu, IPC_MENU_ADDITEM, data_size, req);
		free(req);
	}
	
	return ret;
}
