/*
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

extern tAxWin4_Window	*AxWin4_CreateWindow(const char *Name);
extern void	AxWin4_ShowWindow(tAxWin4_Window *Window);
extern void	AxWin4_SetTitle(tAxWin4_Window *Window, const char *Title);
extern void	AxWin4_DrawBitmap(tAxWin4_Window *Window, int X, int Y, unsigned int W, unsigned int H, void *Data);


#ifdef __cplusplus
}
#endif

#endif

