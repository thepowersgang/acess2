/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * internal.h
 * - Internal definitions
 */
#ifndef _INTERNAL_H_
#define _INTERNAL_H_

struct sAxWin3_Window
{
	uint32_t	ServerID;
	tAxWin3_WindowMessageHandler	Handler;
	
	char	Data[];
};

extern const char	*gsAxWin3_int_ServerDesc;

#endif

