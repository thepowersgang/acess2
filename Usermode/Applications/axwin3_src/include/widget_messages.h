/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * widget_messages.h
 * - AxWin2 Widget port
 */
#ifndef _WIDGET_MESSAGES_H_
#define _WIDGET_MESSAGES_H_

enum eWidgetIPCCalls
{
	// Control (Client->Server) messages
	IPC_WIDGET_CREATE,
	IPC_WIDGET_CREATESUBWIN,
	IPC_WIDGET_DELETE,
	IPC_WIDGET_SETFOCUS,
	IPC_WIDGET_SETFLAGS,
	IPC_WIDGET_SETSIZE,
	IPC_WIDGET_SETTEXT,
	IPC_WIDGET_SETCOLOUR,
	
	IPC_WIDGET_GETTEXT,
	
	N_IPC_WIDGET
};

typedef struct
{
	uint32_t	Parent;
	uint32_t	NewID;
	uint32_t	Type;
	uint32_t	Flags;
	char	DebugName[];
} tWidgetIPC_Create;

typedef struct
{
	uint32_t	Parent;
	uint32_t	NewID;
	uint32_t	Type;
	uint32_t	Flags;
	uint32_t	WindowHandle;
	char	DebugName[];
} tWidgetIPC_CreateSubWin;

typedef struct
{
	uint32_t	WidgetID;
} tWidgetIPC_Delete;

typedef struct
{
	uint32_t	WidgetID;
} tWidgetIPC_SetFocus;

typedef struct
{
	uint32_t	WidgetID;
	uint32_t	Value;
	uint32_t	Mask;
} tWidgetIPC_SetFlags;

typedef struct
{
	uint32_t	WidgetID;
	uint32_t	Value;
} tWidgetIPC_SetSize;

typedef struct
{
	uint32_t	WidgetID;
	char	Text[];
} tWidgetIPC_SetText;

typedef struct
{
	uint32_t	WidgetID;
	uint32_t	Index;
	uint32_t	Colour;
} tWidgetIPC_SetColour;

enum eWidgetMessages
{
	// Event (Server->Client) messages
	MSG_WIDGET_FIRE = 0x1000,
	MSG_WIDGET_KEYPRESS,
	MSG_WIDGET_MOUSEBTN,
};



typedef struct
{
	uint32_t	WidgetID;
} tWidgetMsg_Fire;

typedef struct
{
	uint32_t	WidgetID;
	uint32_t	KeySym;
	uint32_t	Character;
} tWidgetMsg_KeyEvent;

typedef struct
{
	uint32_t	WidgetID;
	uint16_t	X;
	uint16_t	Y;
	uint8_t 	Button;
	uint8_t 	bPressed;
} tWidgetMsg_MouseBtn;

#endif

