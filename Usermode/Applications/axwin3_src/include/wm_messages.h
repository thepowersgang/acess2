/*
 * Acess2 Window Manager v3 (axwin3)
 * - By John Hodge (thePowersGang)
 *
 * include/wm_messages.h
 * - Core window messages
 */
#ifndef _WM_MESSAGES_H_
#define _WM_MESSAGES_H_

/**
 * \brief Messages delivered to windows
 */
enum eWM_WindowMessages
{
	WNDMSG_NULL,	// 0
	
	WNDMSG_CREATE,	// 1
	WNDMSG_DESTROY,	// 2
	WNDMSG_CLOSE,	// 3
	WNDMSG_FOCUS,	// 4 Called on change
	WNDMSG_SHOW,	// 5 Called on change

	WNDMSG_RESIZE,	// 6
	
	WNDMSG_MOUSEMOVE,	// 7
	WNDMSG_MOUSEBTN,	// 8
	WNDMSG_KEYDOWN, 	// 9
	WNDMSG_KEYFIRE, 	// 10
	WNDMSG_KEYUP,   	// 11

	WNDMSG_HOTKEY,
	
	WNDMSG_CLASS_MIN = 0x1000,
	WNDMSG_CLASS_MAX = 0x2000,
};

struct sWndMsg_Bool
{
	 uint8_t	Val;
};

struct sWndMsg_Resize
{
	uint16_t	W, H;
};

struct sWndMsg_MouseMove
{
	 int16_t	X, Y;
	 int16_t	dX, dY;
};

struct sWndMsg_MouseButton
{
	 int16_t	X, Y;
	uint8_t 	Button;
	uint8_t 	bPressed;
};

struct sWndMsg_KeyAction
{
	uint32_t	KeySym;
	uint32_t	UCS32;
};

struct sWndMsg_Hotkey
{
	uint16_t	ID;
};

#endif
