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
tAxWin_MessageCallback	*gAxWin_DefaultCallback;

// === CODE ===
int SoMain()
{
	return 0;
}

tAxWin_Message *AxWin_int_SendAndWait(int RetID, tAxWin_Message *Message)
{
	tAxWin_Message	*msg;
	tAxWin_RetMsg	*rmsg;

	AxWin_SendMessage(Message);
	
	for(;;)
	{
		msg = AxWin_WaitForMessage();
		
		rmsg = (void*)msg->Data;
		if(msg->ID == MSG_SRSP_RETURN && rmsg->ReqID == Message->ID )
			break;
		
		AxWin_HandleMessage(msg);
		free(msg);
	}
	
	return msg;
}

int AxWin_Register(const char *Name, tAxWin_MessageCallback *DefaultCallback)
{
	tAxWin_Message	req;
	tAxWin_Message	*msg;
	 int	ret;
	 int	len = strlen(Name);
	
	req.ID = MSG_SREQ_REGISTER;
	req.Size = 1 + (len+1)/4;
	strcpy(req.Data, Name);
	
	msg = AxWin_int_SendAndWait(MSG_SREQ_ADDTAB, &req);
	ret = ((tAxWin_RetMsg*)msg->Data)->Bool;
	free(msg);

	gAxWin_DefaultCallback = DefaultCallback;
	
	return !!ret;
}

tAxWin_Element *AxWin_CreateTab(const char *Title)
{
	tAxWin_Message	req;
	tAxWin_Message	*msg;
	tAxWin_Element	*ret;
	 int	len = strlen(Title);
	
	req.ID = MSG_SREQ_ADDTAB;
	req.Size = 1 + (len+1)/4;
	strcpy(req.Data, Title);
	
	msg = AxWin_int_SendAndWait(MSG_SRSP_RETURN, &req);
	ret = (tAxWin_Element*) ((tAxWin_RetMsg*)msg->Data)->Handle;
	free(msg);
	
	return ret;
}

tAxWin_Element *AxWin_AddMenuItem(tAxWin_Element *Parent, const char *Label, int Message)
{
	return NULL;
}
