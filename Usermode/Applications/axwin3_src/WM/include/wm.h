/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/wm.h
 * - Window management functions
 */
#ifndef _WM_H_
#define _WM_H_

#include <stdint.h>
#include <stdlib.h>

#include "image.h"

// === CONSTANTS ===
/**
 * \brief Window Flags
 * \{
 */
//! Render the window
#define WINFLAG_SHOW    	0x00000001
//! Don't decoratate even if root
#define WINFLAG_NODECORATE	0x00000002
//! Window takes up all of screen
#define WINFLAG_MAXIMIZED	0x00000004
//! Window is contained within the parent
#define WINFLAG_RELATIVE	0x00000008
//! Window contents are valid
#define WINFLAG_CLEAN    	0x00000040
//! All child windows are un-changed
#define WINFLAG_CHILDCLEAN    	0x00000080

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
extern void	WM_DestroyWindow(tWindow *Window);
extern tWindow	*WM_GetWindowByID(tWindow *Requester, uint32_t ID);
extern void	WM_Invalidate(tWindow *Window);
extern void	WM_SetWindowTitle(tWindow *Window, const char *Title);
extern void	WM_FocusWindow(tWindow *Destination);
extern void	WM_RaiseWindow(tWindow *Window);
extern void	WM_ShowWindow(tWindow *Window, int bShow);
extern void	WM_DecorateWindow(tWindow *Window, int bDecorate);
extern void	WM_SetRelative(tWindow *Window, int bRelativeToParent);
extern int	WM_ResizeWindow(tWindow *Window, int W, int H);
extern int	WM_MoveWindow(tWindow *Window, int X, int Y);
extern int	WM_SendMessage(tWindow *Source, tWindow *Dest, int MessageID, int Length, const void *Data);
// --- Rendering
extern void	WM_Render_FillRect(tWindow *Window, int X, int Y, int W, int H, tColour Colour);
extern void	WM_Render_DrawRect(tWindow *Window, int X, int Y, int W, int H, tColour Colour);
/**
 * \brief Draw text to a window
 * \param Window	Destination Window
 * \param X	X coordinate (Left)
 * \param Y	Y coordinate (Top)
 * \param W	Width of destination region
 * \param H	Height of destination region
 * \param Font	Font to use
 * \param Colour	Text foreground colour
 * \param Text	UTF-8 string to render
 * \param MaxLen	Number of bytes in \a Text to read (Note: A final multi-byte sequence can exceed this count)
 *
 * \note As as noted in the \a MaxLen parameter, up to 3 more bytes may be read
 *       if the final character is a multi-byte UTF-8 sequence. This allows 1
 *       to be passed to only render a single character.
 */
extern int	WM_Render_DrawText(tWindow *Window, int X, int Y, int W, int H, tFont *Font, tColour Colour, const char *Text, int MaxLen);
/**
 * \brief Get the dimensions of a string if it was rendered
 * \param Font	Font to use
 * \param Text	UTF-8 string to be processed
 * \param MaxLen	Number of bytes in \a Text to read (same caveat as WM_Render_DrawText applies)
 * \param W	Pointer to an integer to store the width of the rendered text
 * \param H	Pointer to an integer to store the height of the rendered text
 */
extern void	WM_Render_GetTextDims(tFont *Font, const char *Text, int MaxLen, int *W, int *H);
extern void	WM_Render_DrawImage(tWindow *Window, int X, int Y, int W, int H, tImage *Image);
extern void	WM_Render_SetTextCursor(tWindow *Window, int X, int Y, int W, int H, tColour Colour);
// NOTE: Should really be elsewhere
extern tColour	Video_AlphaBlend(tColour _orig, tColour _new, uint8_t _alpha);
#endif

