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

struct sAxWin3_Window
{
	uint32_t	ServerID;
	tAxWin3_WindowMessageHandler	Handler;
	
	char	Data[];
};

extern void	*AxWin3_int_GetDataPtr(tHWND Window);
extern uint32_t	AxWin3_int_GetWindowID(tHWND Window);

#endif

