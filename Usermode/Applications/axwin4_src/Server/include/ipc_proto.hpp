/*
 * Acess2 GUI v4
 * - By John Hodge (thePowersGang) 
 *
 * ipc_proto.hpp
 * - IPC Protocol Header
 */
#ifndef _IPC_PROTO_H_
#define _IPC_PROTO_H_

namespace AxWin {

enum
{
	IPCMSG_PING,
	IPCMSG_REPLY,
	IPCMSG_CREATEWIN,
	IPCMSG_CLOSEWIN,
	IPCMSG_SETWINATTR,
	IPCMSG_GETWINATTR,
	
	IPCMSG_RGNADD,
	IPCMSG_RGNDEL,
	IPCMSG_RGNSETATTR,
	IPCMSG_RGNPUSHDATA,
	
	IPCMSG_SENDIPC,
};

};

#endif

