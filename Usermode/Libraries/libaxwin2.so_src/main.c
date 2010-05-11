/*
 * AxWin Window Manager Interface Library
 * By John Hodge (thePowersGang)
 * This file is published under the terms of the Acess Licence. See the
 * file COPYING for details.
 * 
 * main.c - Library Initialisation
 */
#include "common.h"
#include <string.h>

// === GLOBALS ===
 int	giAxWin_Mode = 0;
 int	giAxWin_PID = 0;

// === CODE ===
int SoMain()
{
	return 0;
}

int AxWin_Register(const char *Name)
{
	tAxWin_Message	req;
	tAxWin_Message	*msg;
	tAxWin_RetMsg	*ret;
	 int	len = strlen(Name);
	
	req.ID = MSG_SREQ_REGISTER;
	req.Size = 1 + (len+1)/4;
	strcpy(req.Data, Name);
	
	AxWin_SendMessage(&req);
	
	for(;;)
	{
		msg = AxWin_WaitForMessage();
		
		if(msg->ID == MSG_SREQ_ADDTAB)
		{
			ret = (void*) &msg->Data[0];
			if( ret->ReqID == MSG_SREQ_REGISTER )
				break;
		}
		
		AxWin_HandleMessage(msg);
		free(msg);
	}
	
	return !!ret->Bool;
}

tAxWin_Handle AxWin_AddTab(const char *Title)
{
	tAxWin_Message	req;
	tAxWin_Message	*msg;
	tAxWin_RetMsg	*ret;
	 int	len = strlen(Title);
	
	req.ID = MSG_SREQ_ADDTAB;
	req.Size = 1 + (len+1)/4;
	strcpy(req.Data, Title);
	
	for(;;)
	{
		msg = AxWin_WaitForMessage();
		
		if(msg->ID == MSG_SRSP_RETURN)
		{
			ret = (void*) &msg->Data[0];
			if( ret->ReqID == MSG_SREQ_ADDTAB )
				break;
		}
		
		AxWin_HandleMessage(msg);
		free(msg);
	}
	
	return (tAxWin_Handle) ret->Handle;
}
