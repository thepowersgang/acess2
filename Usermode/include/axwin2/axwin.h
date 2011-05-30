/**
 * \file axwin2/axwin.h
 * \author John Hodge (thePowersGang)
 * \brief AxWin Core functions
 */
#ifndef _AXWIN2_AXWIN_H
#define _AXWIN2_AXWIN_H

#include <stdlib.h>
#include <stdint.h>

#include <axwin2/messages.h>

// === Core Types ===
typedef struct sAxWin_Element	tAxWin_Element;
//typedef struct sAxWin_Message	tAxWin_Message;
typedef int	tAxWin_MessageCallback(tAxWin_Message *);

// === Functions ===
extern int	AxWin_Register(const char *ApplicationName, tAxWin_MessageCallback *DefaultHandler);
extern tAxWin_Element	*AxWin_CreateTab(const char *TabTitle);
extern tAxWin_Element	*AxWin_AddMenuItem(tAxWin_Element *Parent, const char *Label, int Message);

extern int	AxWin_MessageLoop(void);
extern int	AxWin_SendMessage(tAxWin_Message *Message);
extern tAxWin_Message	*AxWin_WaitForMessage(void);
extern int	AxWin_HandleMessage(tAxWin_Message *Message);

// === Window Control ===

extern tAxWin_Element	*AxWin_CreateElement(int ElementType);

#endif
