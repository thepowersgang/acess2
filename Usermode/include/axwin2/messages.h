/**
 * \file messages.h
 * \author John Hodge (thePowersGang)
 * \brief AxWin Control Messages and structures
 */
#ifndef _AXWIN_MESSAGES_H
#define _AXWIN_MESSAGES_H

#include <stdint.h>

typedef struct sAxWin_Message	tAxWin_Message;
typedef struct sAxWin_RetMsg	tAxWin_RetMsg;

// Higherarchy:
// - HANDLE
//  + ELEMENT
//   > DIALOG
//   > TAB

/**
 * \brief Message IDs
 */
enum eAxWin_Messages
{
	// Client->Server Requests
	MSG_SREQ_PING,
	// - Windows
	MSG_SREQ_REGISTER,	// bool (char[] Name) - Registers this PID with the Window Manager
	
	MSG_SREQ_ADDWIN,	// ELEMENT (char[] Name) - Adds a tab to the window
	
	MSG_SREQ_SETICON,	// void (TAB Tab, char[] IconURI)	- Set the icon of a tab (or application)

	MSG_SREQ_NEWDIALOG,	// DIALOG (TAB Parent, char[] Name)	- Creates a dialog
	MSG_SREQ_DELDIALOG,	// void (DIALOG Dialog)	- Closes a dialog
	
	MSG_SREQ_SETNAME,	// void (ELEMENT Element, char[] Name)
	MSG_SREQ_GETNAME,	// char[] (ELEMENT Element)
	
	// - Builtin Elements
	MSG_SREQ_INSERT,	// ELEMENT (ELEMENT Parent, eAxWin_Controls Type, u32 Flags)
	MSG_SREQ_DELETE,	// void (ELEMENT Element)
	
	// - Drawing
	//  All drawing functions take an ELEMENT as their first parameter.
	//  This must be either a Window or Canvas control
	MSG_SREQ_SETCOL,
	MSG_SREQ_PSET,
	MSG_SREQ_LINE,	MSG_SREQ_CURVE,
	MSG_SREQ_RECT,	MSG_SREQ_FILLRECT,
	MSG_SREQ_RIMG,	MSG_SREQ_SIMG,	// Register/Set Image
	MSG_SREQ_SETFONT,	MSG_SREQ_PUTTEXT,

	// - Callback Registration

	// - WM Control
	MSG_SREQ_SET_MAXIMIZE_AREA,	// void (uint16_t X, Y, W, H)
	
	// Server->Client Responses
	MSG_SRSP_VERSION,
	MSG_SRSP_RETURN,	// {int RequestID, void[] Return Value} - Returns a value from a server request
	
	NUM_MSG
};

// --- Server Requests (Requests from the client of the server)
/**
 * \brief Server Request - Ping (Get Server Version)
 */
struct sAxWin_SReq_Ping
{
};

/**
 * \brief Server Request - New Window
 * \see eAxWin_Messages.MSG_SREQ_NEWWINDOW
 */
struct sAxWin_SReq_NewWindow
{
	uint16_t	X, Y, W, H;
	uint32_t	Flags;
};

struct sAxWin_SReq_NewElement
{
	uint16_t	Parent;
	uint16_t	Type;
	uint32_t	Flags;
};


// --- Server Responses
/**
 * \brief Server Response - Pong
 * \see eAxWin_Messages.MSG_SRSP_PONG
 */
struct sAxWin_SRsp_Version
{
	uint8_t	Major;
	uint8_t	Minor;
	uint16_t	Build;
};


// === Core Message Structure
/**
 * \brief Overarching Message Structure
 * \note sizeof(tAxWin_Message) is never valid
 */
struct sAxWin_Message
{
	uint32_t	Source;
	uint16_t	ID;
	uint16_t	Size;	// Size of data
	char	Data[];
};

struct sAxWin_RetMsg
{
	uint16_t	ReqID;
	uint16_t	Rsvd;
	uint32_t	Value;
};

#endif
