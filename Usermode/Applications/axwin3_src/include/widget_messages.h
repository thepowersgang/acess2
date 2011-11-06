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
	MSG_WIDGET_CREATE = 0x1000,
	MSG_WIDGET_DELETE,
	MSG_WIDGET_SETFLAGS,
	MSG_WIDGET_SETSIZE,
	MSG_WIDGET_SETTEXT,
	MSG_WIDGET_SETCOLOUR
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

#endif

