/**
 * \file messages.h
 * \author John Hodge (thePowersGang)
 * \brief AxWin Control Messages and structures
 */
#ifndef _AXWIN_MESSAGES_H
#define _AXWIN_MESSAGES_H

#include <stdint.h>

typedef struct sAxWin_Message	tAxWin_Message;

/**
 * \brief Message IDs
 */
enum eAxWin_Messages
{
	// Server Requests
	MSG_SREQ_PING,
	// - Windows
	MSG_SREQ_NEWWINDOW,	// (short x, y, w, h, uint32_t flags)
	MSG_SREQ_GETFLAGS,	MSG_SREQ_SETFLAGS,
	MSG_SREQ_GETRECT,	MSG_SREQ_SETRECT,
	// - Drawing
	MSG_SREQ_SETCOL,
	MSG_SREQ_PSET,
	MSG_SREQ_LINE,	MSG_SREQ_CURVE,
	MSG_SREQ_RECT,	MSG_SREQ_FILLRECT,
	MSG_SREQ_RIMG,	MSG_SREQ_SIMG,	// Register/Set Image
	MSG_SREQ_SETFONT,	MSG_SREQ_PUTTEXT,
	
	// Server Responses
	MSG_SRSP_PONG,
	MSG_SRSP_WINDOW,	// Returns the new window ID
	MSG_SRSP_IMG,	// Returns the image ID
	
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


// --- Server Responses
/**
 * \brief Server Response - Pong
 * \see eAxWin_Messages.MSG_SRSP_PONG
 */
struct sAxWin_SRsp_Pong
{
	uint8_t	Major;
	uint8_t	Minor;
	uint16_t	Build;
};

/**
 * \brief Server Response - New Window
 * \see eAxWin_Messages.MSG_SRSP_NEWWINDOW
 */
struct sAxWin_SRsp_NewWindow
{
	uint32_t	Handle;
};


// === Core Message Structure
/**
 * \brief Overarching Message Structure
 * \note sizeof(tAxWin_Message) is never valid
 */
struct sAxWin_Message
{
	uint16_t	ID;
	uint16_t	Size;	//!< Size in DWORDS
	union
	{
		struct sAxWin_SReq_Ping	SReq_Pong;
		struct sAxWin_SReq_NewWindow	SReq_NewWindow;
		
		// Server Responses
		struct sAxWin_SRsp_Pong	SRsp_Pong;
		struct sAxWin_SRsp_NewWindow	SRsp_Window;
	};
};

#endif
