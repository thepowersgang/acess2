/*
* Acess2 Window Manager v3 (axwin3)
* - By John Hodge (thePowersGang)
*
* include/wm_renderer.h
* - Window renderer API
*/
#ifndef _WM_RENDERER_H_
#define _WM_RENDERER_H_

typedef struct
{
	tWindow	(*InitWindow)(int W, int H, int Flags);
	void	(*Redraw)(tWindow *Window);
	 int	(*SendMessage)(tWindow *Window, int MessageID, int Length, void *Data);
}	tWMRenderer;

#endif
