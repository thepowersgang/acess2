/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/wm.h
 * - Window management functions
 */
#ifndef _WM_H_
#define _WM_H_

#include "image.h"

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
typedef struct sFont	tFont;
typedef struct sIPC_Client	tIPC_Client;

// === FUNCTIONS ===
// --- Management
extern tWindow	*WM_CreateWindow(tWindow *Parent, tIPC_Client *Client, uint32_t ID, int Flags, const char *Renderer);
extern void	WM_Invalidate(tWindow *Window);
extern void	WM_ShowWindow(tWindow *Window, int bShow);
extern int	WM_ResizeWindow(tWindow *Window, int W, int H);
extern int	WM_MoveWindow(tWindow *Window, int X, int Y);
extern int	WM_SendMessage(tWindow *Source, tWindow *Dest, int MessageID, int Length, void *Data);
// --- Rendering
extern void	WM_Render_FillRect(tWindow *Window, int X, int Y, int W, int H, tColour Colour);
extern void	WM_Render_DrawRect(tWindow *Window, int X, int Y, int W, int H, tColour Colour);
extern int	WM_Render_DrawText(tWindow *Window, int X, int Y, int W, int H, tFont *Font, tColour Colour, const char *Text);
extern void	WM_Render_GetTextDims(tFont *Font, const char *Text, int *W, int *H);
extern void	WM_Render_DrawImage(tWindow *Window, int X, int Y, int W, int H, tImage *Image);
// NOTE: Should really be elsewhere
extern tColour	Video_AlphaBlend(tColour _orig, tColour _new, uint8_t _alpha);
#endif

