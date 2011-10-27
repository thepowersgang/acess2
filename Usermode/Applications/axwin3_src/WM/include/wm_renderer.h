/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/wm_renderer.h
 * - Window renderer API
 */
#ifndef _WM_RENDERER_H_
#define _WM_RENDERER_H_

#include <wm.h>
#include <wm_internals.h>

struct sWMRenderer
{
	/**
	 * \brief Internal pointer to next loaded renderer
	 */
	struct sWMRenderer	*Next;

	/**
	 * \brief Renderer name / identifier
	 */
	const char	*Name;

	/**
	 * \brief Initialise a window structure for the renderer
	 * \param W	Width of the new window
	 * \param H	Height of the new window
	 * \param Flags	Flags for the window
	 * \return malloc()'d window structure, or NULL on error
	 * \note Only the fields \a W and \a H need be filled in the window
	 *       structure, the rest will be filled by the caller.
	 */
	tWindow	*(*CreateWindow)(int W, int H, int Flags);

	/**
	 * \brief Redraw a window on the screen
	 * \param Window	Window to render
	 * 
	 * Called when a window needs to be re-rendered, e.g. when it is uncovered or
	 * repositioned.
	 *
	 * \todo List all conditions for Redraw
	 */
	void	(*Redraw)(tWindow *Window);
	
	/**
	 * \brief Handle a message sent to the window using WM_SendMessage
	 * \param Window	Target window
	 * \param MessageID	Implementation defined message ID (usually the command)
	 * \param Length	Length of the buffer \a Data
	 * \param Data  	Implementation defined data buffer
	 * \return Boolean failure (0: Handled, 1: Unhandled)
	 */
	 int	(*HandleMessage)(tWindow *Window, int MessageID, int Length, void *Data);
};

extern void	WM_RegisterRenderer(tWMRenderer *Renderer);
extern tWindow	*WM_CreateWindowStruct(size_t ExtraBytes);

#endif
