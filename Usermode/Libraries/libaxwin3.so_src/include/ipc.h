/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * ipcmessages.h
 * - IPC Message format definition
 */
#ifndef _IPCMESSAGES_H_
#define _IPCMESSAGES_H_

typedef struct sAxWin_IPCMessage	tAxWin_IPCMessage;
typedef struct sIPCMsg_Return	tIPCMsg_Return;

/**
 * \name Flags for IPC Messages
 * \{
 */
//! Request a return value
#define IPCMSG_FLAG_RETURN	0x01

struct sAxWin_IPCMessage
{
	 uint8_t	ID;
	 uint8_t	Flags;
	uint16_t	Size;
	uint32_t	Window;
	char	Data[];
};

struct sIPCMsg_Return
{
	uint32_t	Value;
};

enum eAxWin_IPCMessageTypes
{
	IPCMSG_PING,	//!< 
	IPCMSG_SENDMSG,	//!< Send a message to another window
};

#endif

