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
	IPCMSG_NULL,
	IPCMSG_REPLY,
	IPCMSG_PING,
	IPCMSG_GETGLOBAL,
	IPCMSG_SETGLOBAL,

	IPCMSG_CREATEWIN,
	IPCMSG_CLOSEWIN,
	IPCMSG_SETWINATTR,
	IPCMSG_GETWINATTR,
	IPCMSG_SENDIPC,
	IPCMSG_GETWINBUF,	// get a handle to the window's buffer
	
	// - Window drawing commands
	//IPCMSG_DRAWGROUP,	// (u16 win, u16 group_id) - (hint) Switch to this group
	//IPCMSG_CLEAR,	// (u16 win) - (hint) Clear current drawing group
	IPCMSG_PUSHDATA,	// (u16 win, u16 x, u16 y, u16 w, u16 h, void data)
	IPCMSG_BLIT,	// (win, sx, sy, dx, dy, w, h) - Blit locally
	IPCMSG_DRAWCTL,	// (win, x, y, w, h, ctlid) - Draw
	IPCMSG_DRAWTEXT,	// (win, x, y, fontid, text) - Draw text using an internal font
};

enum eIPC_GlobalAttrs
{
	IPC_GLOBATTR_SCREENDIMS,	// Screen dimensions - Readonly
	IPC_GLOBATTR_MAXAREA,	// Maximum window area for screen (hint only, not enforced)
};

enum eIPC_WinAttrs
{
	IPC_WINATTR_SHOW,	// u8	- Window shown
	IPC_WINATTR_FLAGS,	// u32	- Decoration enabled, always-on-top
	IPC_WINATTR_POSITION,	// s16, s16
	IPC_WINATTR_DIMENSIONS,	// u16, u16
	IPC_WINATTR_TITLE,	// string
};

};

#endif

