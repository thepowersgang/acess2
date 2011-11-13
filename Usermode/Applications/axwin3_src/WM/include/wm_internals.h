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
	tWindow	*NextSibling;
	tWindow	*PrevSibling;

	// Render tree
	tWindow	*Parent;
	tWindow	*FirstChild;
	tWindow	*LastChild;

	tIPC_Client	*Client;
	uint32_t	ID;	//!< Client assigned ID
	tWMRenderer	*Renderer;

	char	*Title;

	 int	Flags;

	 int	BorderL, BorderR;
	 int	BorderT, BorderB;

	 int	RealW, RealH;	

	 int	X, Y;
	 int	W, H;

	void	*RendererInfo;	

	void	*RenderBuffer;	//!< Cached copy of the rendered window
};

#endif

