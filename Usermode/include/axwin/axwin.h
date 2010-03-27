/**
 * \file axwin.h
 * \author John Hodge (thePowersGang)
 * \brief AxWin Core functions
 */
#ifndef _AXWIN_AXWIN_H
#define _AXWIN_AXWIN_H

// === Core Types ===
typedef unsigned int	tAxWin_Handle;

// === Messaging ===
#include "messages.h"
extern int	AxWin_MessageLoop();

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
extern tAxWin_Window	AxWin_CreateWindow(
		int16_t X, int16_t Y, int16_t W, int16_t H,
		uint32_t Flags, tAxWin_MessageCallback *Callback);

#endif
