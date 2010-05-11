/*
 * AxWin Window Manager Interface Library
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess Licence. See the
 * file COPYING for details.
 * 
 * messages.c - Message Handling
 */
#include "common.h"

// === PROTOTYPES ===
 int	AxWin_MessageLoop();
tAxWin_Message	*AxWin_WaitForMessage();
 int	AxWin_HandleMessage(tAxWin_Message *Message);

// ===  ===

// === CODE ===
int AxWin_SendMessage(tAxWin_Message *Message)
{
	switch(giAxWin_Mode)
	{
	case AXWIN_MODE_IPC:
		SysSendMessage(giAxWin_PID, Message->Size*4, Message);
		break;
	default:
		break;
	}
	return 0;
}

/**
 * \brief Loop forever, checking and waiting for messages
 */
int AxWin_MessageLoop()
{
	tAxWin_Message	*msg;
	 int	ret;
	for(;;)
	{
		msg = AxWin_WaitForMessage();
		ret = AxWin_HandleMessage(msg);
		
		if(ret < 0)	return 0;
	}
	return 0;
}

/**
 * \brief Wait for a message
 */
tAxWin_Message *AxWin_WaitForMessage()
{
	 int	length;
	pid_t	src;
	tAxWin_Message	*ret;
	
	switch( giAxWin_Mode )
	{
	case AXWIN_MODE_IPC:
		while( (length = SysGetMessage(&src, NULL)) == 0 )	sleep();
		ret = malloc(length);
		SysGetMessage(NULL, ret);
		break;
	default:
		break;
	}
	return 0;
}

/**
 * \brief Handles a recieved message
 */
int AxWin_HandleMessage(tAxWin_Message *Message)
{
	switch(Message->ID)
	{
	default:	return 0;
	}
}
