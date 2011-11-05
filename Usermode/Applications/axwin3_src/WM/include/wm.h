/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/wm.h
 * - Window management functions
 */
#ifndef _WM_H_
#define _WM_H_

// === CONSTANTS ===
/**
 * \brief Window Flags
 * \{
 */
//! Render the window
#define WINFLAG_SHOW    	0x00000001
//! Window contents are valid
#define WINFLAG_CLEAN    	0x00000002

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
extern int	WM_Reposition(tWindow *Window, int X, int Y, int W, int H);
extern int	WM_SetFlags(tWindow *Window, int Flags);
extern int	WM_SendMessage(tWindow *Window, int MessageID, int Length, void *Data);
// --- Rendering
extern void	WM_Render_FilledRect(tWindow *Window, tColour Colour, int X, int Y, int W, int H);

#endif

