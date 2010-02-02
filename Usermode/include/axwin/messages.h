/**
 * \file messages.h
 * \author John Hodge (thePowersGang)
 * \brief AxWin Control Messages and structures
 */
#ifndef _AXWIN_MESSAGES_H
#define _AXWIN_MESSAGES_H

typedef struct sAxWin_Message	tAxWin_Message;

/**
 * \brief Message IDs
 */
enum eAxWin_Messages
{
	// Server Requests
	MSG_SREQ_PING,
	// - Windows
	MSG_SREQ_NEWWINDOW,
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
	MSG_SRSP_IMG,	// Returns the image ID
	
	NUM_MSG
};

/**
 * \brief New Window Request structure
 */
struct sAxWin_Req_NewWindow
{
	
};

/**
 * \brief Overarching Message Structure
 * \note sizeof(tAxWin_Message) is never valid
 */
struct sAxWin_Message
{
	uint16_t	ID;
	uint16_t	Size;	//!< Size in DWORDS
};

#endif
