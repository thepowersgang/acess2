/*
 * Acess2 GUI Version 3 (AxWin3)
 * - By John Hodge (thePowersGang)
 *
 * axwin.h
 * - Core API Header
 */
#ifndef _AXWIN3_AXWIN_H_
#define _AXWIN3_AXWIN_H_

typedef struct sAxWin3_Window	*tHWND;

typedef void	(*tAxWin3_MessageCallback)(int SourceTID, int Length);

typedef int	(*tAxWin3_WindowMessageHandler)(tHWND Window, int Length, void *Data);

extern void	AxWin3_Connect(const char *ServerDesc);
extern tAxWin3_MessageCallback	AxWin3_SetMessageCallback(tAxWin3_MessageCallback Callback);
extern void	AxWin3_MainLoop(void);

extern tHWND	AxWin3_CreateWindow(tHWND Parent, const char *Renderer, int Flags, int DataBytes, void **DataPtr);
extern void	AxWin3_DestroyWindow(tHWND Window);

extern void	AxWin3_SendMessage(tHWND Window, int Length, void *Data);
extern void	AxWin3_SetWindowPos(tHWND Window, int X, int Y, int W, int H);
extern void	AxWin3_SetWindowShown(tHWND Window, int bShow);

#endif

