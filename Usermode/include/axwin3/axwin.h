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
typedef unsigned int	tAxWin3_Colour;	// TODO: Actual 32-bit

typedef void	(*tAxWin3_MessageCallback)(int SourceTID, int Length);

typedef int	(*tAxWin3_WindowMessageHandler)(tHWND Window, int Message, int Length, void *Data);

// --- Connection management
extern void	AxWin3_Connect(const char *ServerDesc);
extern tAxWin3_MessageCallback	AxWin3_SetMessageCallback(tAxWin3_MessageCallback Callback);
extern void	AxWin3_MainLoop(void);

// --- Window creation/deletion
/**
 * \brief Create a new window (with the required client structures)
 * \param Parent	Parent window handle
 * \param Renderer	Symbolic name of the renderer to use
 * \param RendererArg	Argument to pass to the renderer's initialisation
 * \param DataBytes	Number of bytes to allocate for the caller's use
 * \param MessageHandler	Function to call when a message arrives for the window
 * \return New window handle
 * \note Usually wrapped by renderer-specific functions
 */
extern tHWND	AxWin3_CreateWindow(
	tHWND Parent,
	const char *Renderer, int RendererArg,
	int DataBytes,
	tAxWin3_WindowMessageHandler MessageHandler
	);
/**
 * \brief Destroy a window
 * \param Window	Handle to a window to destroy
 */
extern void	AxWin3_DestroyWindow(tHWND Window);

// --- Core window management functions
extern void	AxWin3_SendMessage(tHWND Window, tHWND Dest, int Message, int Length, void *Data);
extern void	AxWin3_ShowWindow(tHWND Window, int bShow);
extern void	AxWin3_SetWindowPos(tHWND Window, short X, short Y, short W, short H);
extern void	AxWin3_MoveWindow(tHWND Window, short X, short Y);
extern void	AxWin3_ResizeWindow(tHWND Window, short W, short H);

#endif

