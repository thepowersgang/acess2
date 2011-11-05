/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/wm.h
 * - Window management functions
 */
#ifndef _WM_H_
#define _WM_H_

#include <image.h>

// === CONSTANTS ===
/**
 * \brief Window Flags
 * \{
 */
//! Render the window
#define WINFLAG_SHOW    	0x00000001
//! Window contents are valid
#define WINFLAG_CLEAN    	0x00000002
//! All child windows are un-changed
#define WINFLAG_CHILDCLEAN    	0x00000004

#define WINFLAG_RENDER_MASK	0x00FFFF00
#define WINFLAG_USR_MASK	0xFF000000
/**
 * \}
 */

// === TYPES ===
typedef struct sWindow	tWindow;
typedef struct sWMRenderer	tWMRenderer;
typedef uint32_t	tColour;

// === FUNCTIONS ===
// --- Management
extern tWindow	*WM_CreateWindow(tWindow *Parent, int Flags, const char *Renderer);
extern void	WM_Invalidate(tWindow *Window);
extern void	WM_ShowWindow(tWindow *Window, int bShow);
extern int	WM_ResizeWindow(tWindow *Window, int W, int H);
extern int	WM_MoveWindow(tWindow *Window, int X, int Y);
extern int	WM_SendMessage(tWindow *Source, tWindow *Dest, int MessageID, int Length, void *Data);
// --- Rendering
extern void	WM_Render_FillRect(tWindow *Window, int X, int Y, int W, int H, tColour Colour);
extern void	WM_Render_DrawRect(tWindow *Window, int X, int Y, int W, int H, tColour Colour);
extern void	WM_Render_DrawText(tWindow *Window, int X, int Y, int W, int H, void *Font, tColour Colour, const char *Text);
extern void	WM_Render_DrawImage(tWindow *Window, int X, int Y, int W, int H, tImage *Image);

#endif

