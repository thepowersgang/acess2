/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * ipcmessages.h
 * - IPC Message format definition
 */
#ifndef _IPCMESSAGES_H_
#define _IPCMESSAGES_H_

typedef struct sAxWin_IPCMessage tAxWin_IPCMessage;

/**
 * \name Flags for IPC Messages
 * \{
 */
//! Request a return value
#define IPCMSG_FLAG_RETURN	1

struct sAxWin_IPCMessage
{
	uint16_t	ID;
	uint16_t	Flags;
	char	Data[];
};

enum eAxWin_IPCMessageTypes
{
	IPCMSG_PING,	//!< 
	IPCMSG_SENDMSG,	//!< Send a message to another window
};

#endif

