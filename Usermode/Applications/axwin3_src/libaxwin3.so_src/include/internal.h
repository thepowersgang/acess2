/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * internal.h
 * - Internal definitions
 */
#ifndef _INTERNAL_H_
#define _INTERNAL_H_

#include <stdint.h>

#include <acess/sys.h>	// _SysDebug

struct sAxWin3_Window
{
	uint32_t	ServerID;
	tAxWin3_WindowMessageHandler	Handler;
	
	char	Data[];
};

extern void	*AxWin3_int_GetDataPtr(tHWND Window);
extern uint32_t	AxWin3_int_GetWindowID(tHWND Window);
extern void	AxWin3_SendIPC(tHWND Window, int Message, size_t Length, const void *Data);
extern void	*AxWin3_WaitIPCMessage(tHWND Window, int Message, size_t *Length);

#endif

