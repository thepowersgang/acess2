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

	tWindow	*Parent;

	tWindow	*FirstChild;
	tWindow	*LastChild;
	
	tWMRenderer	*Renderer;

	 int	Flags;
	
	 int	X;
	 int	Y;
	 int	W;
	 int	H;

	void	*RendererInfo;	

	void	*RenderBuffer;	//!< Cached copy of the rendered window
};

#endif

