/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * widget_messages.h
 * - AxWin2 Widget port
 */
#ifndef _WIDGET_MESSAGES_H_
#define _WIDGET_MESSAGES_H_

enum
{
	// Control (Client->Server) messages
	MSG_WIDGET_CREATE = 0x1000,
	MSG_WIDGET_DELETE,
	MSG_WIDGET_SETFOCUS,
	MSG_WIDGET_SETFLAGS,
	MSG_WIDGET_SETSIZE,
	MSG_WIDGET_SETTEXT,
	MSG_WIDGET_SETCOLOUR,
	
	// Event (Server->Client) messages
	MSG_WIDGET_FIRE,
	MSG_WIDGET_KEYPRESS,
	MSG_WIDGET_MOUSEBTN,
};


typedef struct
{
	uint32_t	Parent;
	uint32_t	NewID;
	uint32_t	Type;
	uint32_t	Flags;
	char	DebugName[];
} tWidgetMsg_Create;

typedef struct
{
	uint32_t	WidgetID;
} tWidgetMsg_Delete;

typedef struct
{
	uint32_t	WidgetID;
} tWidgetMsg_SetFocus;

typedef struct
{
	uint32_t	WidgetID;
	uint32_t	Value;
	uint32_t	Mask;
} tWidgetMsg_SetFlags;

typedef struct
{
	uint32_t	WidgetID;
	uint32_t	Value;
} tWidgetMsg_SetSize;

typedef struct
{
	uint32_t	WidgetID;
	char	Text[];
} tWidgetMsg_SetText;

typedef struct
{
	uint32_t	WidgetID;
	uint32_t	Index;
	uint32_t	Colour;
} tWidgetMsg_SetColour;

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

