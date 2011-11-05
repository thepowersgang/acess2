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

	info->nElements = 1;
	info->Elements = malloc(sizeof(tAxWin3_Widget*));
	info->Elements[0] = (void*)(info + 1);	// Get end of *info

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
	return info->Elements[0];
}

tAxWin3_Widget *AxWin3_Widget_AddWidget(tAxWin3_Widget *Parent, int Type, int Flags, const char *DebugName)
{
	// Assign ID
	
	return NULL;
}
