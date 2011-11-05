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
	WNDMSG_NULL,
	
	WNDMSG_CREATE,
	WNDMSG_DESTROY,
	WNDMSG_FOCUS,	// Called on change
	WNDMSG_SHOW,	// Called on change

	WNDMSG_RESIZE,
	
	WNDMSG_MOUSEMOVE,
	WNDMSG_MOUSEBTN,
	WNDMSG_KEYDOWN,
	WNDMSG_KEYFIRE,
	WNDMSG_KEYUP,
	
	WNDMSG_CLASS_MIN = 0x1000,
	WNDMSG_CLASS_MAX = 0x2000,
};

struct sWndMsg_Resize
{
	uint16_t	W, H;
};

#endif
