/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * ipcmessages.h
 * - IPC Message format definition
 * - Shared between library and server
 */
#ifndef _IPCMESSAGES_H_
#define _IPCMESSAGES_H_

#include <stdint.h>

typedef struct sAxWin_IPCMessage	tAxWin_IPCMessage;
typedef struct sIPCMsg_Return	tIPCMsg_Return;
typedef struct sIPCMsg_CreateWin	tIPCMsg_CreateWin;
typedef struct sIPCMsg_ShowWindow	tIPCMsg_ShowWindow;
typedef struct sIPCMsg_SetWindowPos	tIPCMsg_SetWindowPos;

/**
 * \name Flags for IPC Messages
 * \{
 */
//! Request a return value
#define IPCMSG_FLAG_RETURN	0x01
/**
 * \}
 */

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

struct sIPCMsg_CreateWin
{
	uint32_t	NewWinID;
	uint32_t	RendererArg;
	char	Renderer[];
};

struct sIPCMsg_ShowWindow
{
	uint32_t	bShow;
};

struct sIPCMsg_SetWindowPos
{
	 int16_t	X;
	 int16_t	Y;
	uint16_t	W;
	uint16_t	H;
	uint8_t 	Fields;
};

enum eAxWin_IPCMessageTypes
{
	IPCMSG_PING,	//!< Get the server version
	IPCMSG_SENDMSG, 	//!< Send a message to another window (or to self)
	IPCMSG_CREATEWIN,	//!< Create a window
	IPCMSG_DESTROYWIN,	//!< Destroy a window
	IPCMSG_SHOWWINDOW,	//!< Show/Hide a window
	IPCMSG_SETWINPOS,	//!< Set a window position
};

#endif
