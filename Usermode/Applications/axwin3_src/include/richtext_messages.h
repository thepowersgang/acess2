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

enum eRichText_IPCCalls
{
	IPC_RICHTEXT_SETATTR,
	IPC_RICHTEXT_SETFONT,
	IPC_RICHTEXT_DELLINE,
	IPC_RICHTEXT_ADDLINE,
	IPC_RICHTEXT_WRITELINE,	// Set line contents
	IPC_RICHTEXT_READLINE,	// Request line contents
	N_IPC_RICHTEXT
};

struct sRichTextIPC_SetAttr
{
	uint32_t	Attr;
	uint32_t	Value;
};

struct sRichTextIPC_SetFont
{
	uint16_t	Size;
	char	Name[];
};

struct sRichTextIPC_AddDelLine
{
	uint32_t	Line;
};

struct sRichTextIPC_ReadLine
{
	uint32_t	Line;
};

struct sRichTextIPC_WriteLine
{
	uint32_t	Line;
	char	LineData[];
};

enum
{
	// Events
	MSG_RICHTEXT_KEYPRESS = 0x1000,
	MSG_RICHTEXT_MOUSEBTN,

	// Sent by server to get a line that is not cached (expects IPC WRITELINE)
	MSG_RICHTEXT_REQLINE,
	// Response to IPC READLINE
	MSG_RICHTEXT_LINEDATA,
};

struct sRichTextMsg_ReqLine
{
	uint32_t	Line;
};

struct sRichTextMsg_LineData
{
	uint32_t	Line;
	char	LineData[];
};

#endif

