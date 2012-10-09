/*
 * Acess2 Window Manager v3
 * - By John Hodge (thePowersGang)
 *
 * richtext_messages.h
 * - Formatted Text Field
 */
#ifndef _RICHTEXT_MESSAGES_H_
#define _RICHTEXT_MESSAGES_H_

enum eRichText_Attrs {
	_ATTR_DEFBG,
	_ATTR_DEFFG,
	_ATTR_SCROLL,
	_ATTR_LINECOUNT,
	_ATTR_COLCOUNT,
	_ATTR_CURSOR,
	_ATTR_CURSORBLINK,
	_ATTR_CURSORPOS,
};

enum
{
	// Calls
	MSG_RICHTEXT_SETATTR,
	MSG_RICHTEXT_SETFONT,
	MSG_RICHTEXT_DELLINE,
	MSG_RICHTEXT_ADDLINE,
	
	// Events
	MSG_RICHTEXT_KEYPRESS,
	MSG_RICHTEXT_MOUSEBTN,

	// Bi-directional messages
	// - Sent by server to get a line that is not cached
	// - Sent by client to read line contents
	MSG_RICHTEXT_REQLINE,
	// - Response to _REQLINE
	MSG_RICHTEXT_SENDLINE,
};

struct sRichTextMsg_SetAttr
{
	uint32_t	Attr;
	uint32_t	Value;
};

struct sRichTextMsg_SetFont
{
	uint16_t	Size;
	char	Name[];
};

struct sRichTextMsg_AddDelLine
{
	uint32_t	Line;
};

struct sRichTextMsg_ReqLine
{
	uint32_t	Line;
};

struct sRichTextMsg_SendLine
{
	uint32_t	Line;
	char	LineData[];
};

#endif

