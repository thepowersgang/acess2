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

// === STRUCTURES ===
struct sAxWin3_Widget
{
	tHWND	Window;
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
		sizeof(*info), AxWin3_Widget_MessageHandler
		);
	info = AxWin3_int_GetDataPtr(ret);
	
	return ret;
}

