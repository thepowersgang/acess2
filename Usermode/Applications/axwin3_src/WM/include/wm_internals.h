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
	uint32_t	ID;
	tWMRenderer	*Renderer;
	
	 int	X;
	 int	Y;
	 int	W;
	 int	H;

	void	*RendererInfo;	

};

#endif

