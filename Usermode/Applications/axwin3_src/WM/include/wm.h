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
#define WINFLAG_SHOW    	0x00000001
#define WINFLAG_RENDER_MASK	0x00FFFF00
#define WINFLAG_USR_MASK	0xFF000000
/**
 * \}
 */

// === FUNCTIONS ===
extern tWindow	*WM_CreateWindow(tWindow *Parent, int X, int Y, int W, int H, int Flags, tRenderer *Handler);
extern int	WM_Reposition(tWindow *Window, int X, int Y, int W, int H);
extern int	WM_SetFlags(tWindow *Window, int Flags);
extern int	WM_SendMessage(tWindow *Window, int MessageID, int Length, void *Data);

#endif

