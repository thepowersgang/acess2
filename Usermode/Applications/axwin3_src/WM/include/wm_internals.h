/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/wm_internals.h
 * - Window management internal definitions
 */
#ifndef _WM_INTERNALS_H_
#define _WM_INTERNALS_H_

#include <wm.h>

struct sWindow
{
	tWindow	*Owner;

	// Render tree
	tWindow	*Parent;
	tWindow	*FirstChild;
	tWindow	*LastChild;
	tWindow	*NextSibling;
	tWindow	*PrevSibling;

	tIPC_Client	*Client;
	uint32_t	ID;	//!< Client assigned ID

	tWMRenderer	*Renderer;
	void	*RendererInfo;	

	char	*Title;

	 int	Flags;

	// Text Cursor
	 int	CursorX, CursorY;
	 int	CursorW, CursorH;

	// Gutter sizes (cached from decorator)
	 int	BorderL, BorderR;
	 int	BorderT, BorderB;

	// Position and dimensions
	 int	X, Y;
	 int	W, H;
	 int	RealX, RealY;
	 int	RealW, RealH;

	void	*RenderBuffer;	//!< Cached copy of the rendered window
};

#endif

