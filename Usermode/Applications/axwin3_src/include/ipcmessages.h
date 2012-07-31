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
typedef struct sIPCMsg_ReturnInt	tIPCMsg_ReturnInt;
typedef struct sIPCMsg_CreateWin	tIPCMsg_CreateWin;
typedef struct sIPCMsg_Boolean	tIPCMsg_Boolean;
typedef struct sIPCMsg_SetWindowPos	tIPCMsg_SetWindowPos;
typedef struct sIPCMsg_SendMsg	tIPCMsg_SendMsg;
typedef struct sIPCMsg_RegAction	tIPCMsg_RegAction;

typedef struct sIPCMsg_GetDisplayDims	tIPCMsg_GetDisplayDims;
typedef struct sIPCMsg_RetDisplayDims	tIPCMsg_RetDisplayDims;

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

struct sIPCMsg_ReturnInt
{
	uint32_t	Value;
};

struct sIPCMsg_CreateWin
{
	uint32_t	NewWinID;
	uint32_t	RendererArg;
	char	Renderer[];
};

struct sIPCMsg_SendMsg
{
	uint32_t	Remote;	// Dest/Source for Server/Client bound
	uint16_t	ID;
	uint16_t	Length;
	char	Data[];
};

struct sIPCMsg_Boolean
{
	uint32_t	Value;
};

struct sIPCMsg_SetWindowPos
{
	 int16_t	X, Y;
	uint16_t	W, H;
	uint8_t 	bSetPos;
	uint8_t 	bSetDims;
};

struct sIPCMsg_RegAction
{
	uint16_t	Index;
	char	Action[];
};

struct sIPCMsg_GetDisplayDims
{
	uint16_t	DisplayID;
};

struct sIPCMsg_RetDisplayDims
{
	uint16_t	X;
	uint16_t	Y;
	uint16_t	W;
	uint16_t	H;
};

enum eAxWin_IPCMessageTypes
{
	IPCMSG_PING,	//!< Get the server version
	IPCMSG_GETDISPLAYCOUNT,	
	IPCMSG_GETDISPLAYDIMS,

	IPCMSG_SENDMSG, 	//!< Send a message to another window (or to self)
	IPCMSG_CREATEWIN,	//!< Create a window
	IPCMSG_DESTROYWIN,	//!< Destroy a window
	IPCMSG_SETWINTITLE,	//!< Set window title
	IPCMSG_SHOWWINDOW,	//!< Show/Hide a window
	IPCMSG_DECORATEWINDOW,	//!< Enable/Disable decorations
	IPCMSG_FOCUSWINDOW,	//!< Give a window focus (no data)
	IPCMSG_SETWINPOS,	//!< Set a window position
	IPCMSG_REGACTION	//!< Register an action name
};

#endif
