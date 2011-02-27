/**
 * \file axwin.h
 * \author John Hodge (thePowersGang)
 * \brief AxWin Core functions
 */
#ifndef _AXWIN_AXWIN_H
#define _AXWIN_AXWIN_H

// === Core Types ===
typedef void	*tAxWin_Handle;

// === Messaging ===
#include "messages.h"

extern int	AxWin_Register(const char *Name);

extern int	AxWin_MessageLoop();
extern int	AxWin_SendMessage(tAxWin_Message *Message);
extern tAxWin_Message	*AxWin_WaitForMessage(void);
extern int	AxWin_HandleMessage(tAxWin_Message *Message);

// === Window Control ===
/**
 * \brief Window Type
 * \note Opaque Type
 */
typedef struct sAxWin_Window	tAxWin_Window;

typedef int	tAxWin_MessageCallback(tAxWin_Message *);

/**
 * \brief Window Flags
 * \{
 */
#define WINFLAG_NOBORDER	0x100
/**
 * \}
 */

extern tAxWin_Window	*AxWin_CreateWindow(
		int16_t X, int16_t Y, int16_t W, int16_t H,
		uint32_t Flags, tAxWin_MessageCallback *Callback);

#endif
