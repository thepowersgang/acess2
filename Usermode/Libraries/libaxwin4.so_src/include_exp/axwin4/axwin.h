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

extern void	AxWin4_DrawBitmap(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H, void *Data);

#include "definitions.h"


#ifdef __cplusplus
}
#endif

#endif

