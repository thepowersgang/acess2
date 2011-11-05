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
	MSG_WIDGET_CREATE,
	MSG_WIDGET_DELETE,
	MSG_WIDGET_SETTEXT
};


typedef struct
{
	uint32_t	Parent;
	uint32_t	NewID;
	char	DebugName[];
} tWidgetMsg_Create;

#endif

