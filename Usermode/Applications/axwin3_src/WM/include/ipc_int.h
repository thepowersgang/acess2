/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 * 
 * ipc.h
 * - Interprocess communication
 */
#ifndef _IPC_INT_H_
#define _IPC_INT_H_

#include <stddef.h>
#include <ipcmessages.h>
#include <wm.h>

typedef struct sIPC_Type	tIPC_Type;

struct sIPC_Type
{
	 int	(*GetIdentSize)(const void *Ident);
	 int	(*CompareIdent)(const void *Ident1, const void *Ident2);
	void	(*SendMessage)(const void *Ident, size_t Length, const void *Data);
};

struct sIPC_Client
{
	const tIPC_Type	*IPCType;
	const void	*Ident;	// Stored after structure

	 int	nWindows;
	tWindow	**Windows;
};

extern int	giIPC_ClientCount;
extern tIPC_Client	**gIPC_Clients;

extern tIPC_Client	*IPC_int_GetClient(const tIPC_Type *IPCType, const void *Ident);
extern void	IPC_int_DropClient(tIPC_Client *Client);
extern void	IPC_Handle(tIPC_Client *Client, size_t MsgLen, tAxWin_IPCMessage *Msg);

#endif
