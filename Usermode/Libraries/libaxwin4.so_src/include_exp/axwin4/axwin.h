/*
 * Acess2 GUIv4 (AxWin4)
 * - By John Hodge (thePowersGang)
 *
 * axwin4/axwin.h
 * - Client library interface header
 */
#ifndef _LIBAXWIN4_AXWIN4_AXWIN_H_
#define _LIBAXWIN4_AXWIN4_AXWIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <acess/sys.h>

typedef struct sAxWin4_Window	tAxWin4_Window;

// - Abstractions of core IPC methods
extern bool	AxWin4_Connect(const char *URI);

extern bool	AxWin4_WaitEventQueue(uint64_t Timeout);	
extern bool	AxWin4_WaitEventQueueSelect(int nFDs, fd_set *rfds, fd_set *wfds, fd_set *efds, uint64_t Timeout);

extern void	AxWin4_GetScreenDimensions(unsigned int ScreenIndex, unsigned int *Width, unsigned int *Height);

extern tAxWin4_Window	*AxWin4_CreateWindow(const char *Name);
extern void	AxWin4_DestroyWindow(tAxWin4_Window *Window);
extern void	AxWin4_ShowWindow(tAxWin4_Window *Window, bool Shown);
extern void	AxWin4_SetWindowFlags(tAxWin4_Window *Window, unsigned int NewFlags);
extern void	AxWin4_SetTitle(tAxWin4_Window *Window, const char *Title);
extern void	AxWin4_MoveWindow(tAxWin4_Window *Window, int X, int Y);
extern void	AxWin4_ResizeWindow(tAxWin4_Window *Window, unsigned int W, unsigned int H);

extern void	AxWin4_DamageRect(tAxWin4_Window *Window, unsigned int X, unsigned int Y, unsigned int W, unsigned int H);
extern void*	AxWin4_GetWindowBuffer(tAxWin4_Window *Window);

/**
 * \brief Set the render clipping region. Any attempts to render outside this area will silently fail
 * \param Window	Target window
 *
 * \note Allows clipping other render functions to avoid excessive redraws
 * \note Cleared when \a AxWin4_DamageRect is called, or when called with a zero width or height
 */
extern void	AxWin4_SetRenderClip(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H);

/**
 * \brief Draw a user-supplied bitmap to the window
 * \param Data	Bitmap data in the same format as the window's back buffer
 * \note VERY SLOW
 */
extern void	AxWin4_DrawBitmap(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H, void *Data);
/**
 * \brief Draw a "control" to the window
 * \param Window	Target window
 * \param X	Destination X
 * \param Y	Destination Y
 * \param W	Control width
 * \param H	Control height
 * \param ControlID	Specifies which control to use. Can be a global or application-registered (See eAxWin4_GlobalControls)
 * \param Frame	Control frame number. Used to specify a variant of the control (e.g. hovered/pressed)
 *
 * Controls are server-side bitmaps that can be arbitarily scaled to fit a region.
 */
extern void	AxWin4_DrawControl(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H, uint16_t ControlID, unsigned int Frame);

extern void	AxWin4_DrawText(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H, uint16_t FontID, const char *String);

#include "definitions.h"

#ifdef __cplusplus
}
#endif

#endif

