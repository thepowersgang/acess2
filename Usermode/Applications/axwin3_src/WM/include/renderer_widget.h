/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * render_widget.h
 * - AxWin2 Widget port
 */
#ifndef _RENDERER_WIDGET_H_
#define _RENDERER_WIDGET_H_

#include <wm_renderer.h>
#include <axwin3/widget.h>

enum
{
	MSG_WIDGET_CREATE,
	MSG_WIDGET_DELETE,
	MSG_WIDGET_SETTEXT
};


typedef struct
{
	uint32_t	Parent;
	uint32_t	NewID;
	char	DebugName[];
} tWidgetMsg_Create;

#endif

