/*
 * AxWin3 Interface Library
 * - By John Hodge (thePowersGang)
 *
 * r_richtext.c
 * - Formatted Text window type
 */
#include <axwin3/axwin.h>
#include <axwin3/richtext.h>
#include "include/internal.h"
#include <richtext_messages.h>
#include <string.h>
#include <wm_messages.h>
//#include <alloca.h>

// === TYPES ===
typedef struct sRichText_Window
{
	tAxWin3_RichText_KeyHandler	KeyCallback;
	tAxWin3_RichText_MouseHandler	MouseCallback;
	tAxWin3_RichText_LineHandler	LineHandler;
} tRichText_Window;

// === CODE ===
int AxWin3_RichText_MessageHandler(tHWND Window, int MessageID, int Size, void *Data)
{
	tRichText_Window	*info = AxWin3_int_GetDataPtr(Window);
	struct sWndMsg_KeyAction	*keyaction = Data;
	switch(MessageID)
	{
	case WNDMSG_KEYFIRE:
		if(Size < sizeof(*keyaction))	return -1;
		info->KeyCallback(Window, 2, keyaction->KeySym, keyaction->UCS32);
		return 1;
	case WNDMSG_KEYDOWN:
		if(Size < sizeof(*keyaction)) 	return -1;
		info->KeyCallback(Window, 1, keyaction->KeySym, keyaction->UCS32);
		return 1;
	case WNDMSG_KEYUP:
		if(Size < sizeof(*keyaction)) 	return -1;
		info->KeyCallback(Window, 0, keyaction->KeySym, 0);
		return 1;
	}
	return 0;
}

static void _SendAttrib(tHWND Window, int Attr, uint32_t Value)
{
	struct sRichTextIPC_SetAttr	msg;
	msg.Attr = Attr;
	msg.Value = Value;
	AxWin3_SendIPC(Window, IPC_RICHTEXT_SETATTR, sizeof(msg), &msg);
}

tHWND AxWin3_RichText_CreateWindow(tHWND Parent, int Flags)
{
	tHWND ret = AxWin3_CreateWindow(Parent, "RichText", Flags,
		sizeof(tRichText_Window), AxWin3_RichText_MessageHandler);
//	tRichText_Window *info = AxWin3_int_GetDataPtr(ret);
	return ret;
}

void AxWin3_RichText_SetKeyHandler(tHWND Window, tAxWin3_RichText_KeyHandler Handler)
{
	tRichText_Window	*info = AxWin3_int_GetDataPtr(Window);
	info->KeyCallback = Handler;
}

void AxWin3_RichText_SetMouseHandler(tHWND Window, tAxWin3_RichText_MouseHandler Handler)
{
	tRichText_Window	*info = AxWin3_int_GetDataPtr(Window);
	info->MouseCallback = Handler;
}

void AxWin3_RichText_EnableScroll(tHWND Window, int bEnable)
{
	_SendAttrib(Window, _ATTR_SCROLL, bEnable);
}
void AxWin3_RichText_SetLineCount(tHWND Window, int Lines)
{
	_SendAttrib(Window, _ATTR_LINECOUNT, Lines);
}
void AxWin3_RichText_SetColCount(tHWND Window, int Cols)
{
	_SendAttrib(Window, _ATTR_COLCOUNT, Cols);
}
void AxWin3_RichText_SetBackground(tHWND Window, uint32_t ARGB_Colour)
{
	_SendAttrib(Window, _ATTR_DEFBG, ARGB_Colour);
}
void AxWin3_RichText_SetDefaultColour(tHWND Window, uint32_t ARGB_Colour)
{
	_SendAttrib(Window, _ATTR_DEFFG, ARGB_Colour);
}
void AxWin3_RichText_SetFont(tHWND Window, const char *FontName, int PointSize)
{
	// TODO: Send message
}
void AxWin3_RichText_SetCursorType(tHWND Window, int Type)
{
	_SendAttrib(Window, _ATTR_CURSOR, Type);
}
void AxWin3_RichText_SetCursorBlink(tHWND Window, int bBlink)
{
	_SendAttrib(Window, _ATTR_CURSORBLINK, bBlink);
}
void AxWin3_RichText_SetCursorPos(tHWND Window, int Row, int Column)
{
	if(Row < 0 || Row > 0xFFFFF || Column > 0xFFF || Column < 0)
		return ;
	_SendAttrib(Window, _ATTR_CURSORPOS, ((Row & 0xFFFFF) << 12) | (Column & 0xFFF));
}

void AxWin3_RichText_ScrollRange(tHWND Window, int FirstRow, int RangeCount, int DownCount)
{
	if( FirstRow < 0 )
		return ;
	if( RangeCount < -2 || RangeCount == 0 )
		return ;
	if( RangeCount > 0 && DownCount > RangeCount )
		return ;

	struct sRichTextIPC_ScrollRange	msg;
	msg.First = FirstRow;
	msg.Range = RangeCount;
	msg.Count = DownCount;
	AxWin3_SendIPC(Window, IPC_RICHTEXT_SCROLLRANGE, sizeof(msg), &msg);
}

void AxWin3_RichText_SendLine(tHWND Window, int Line, const char *Text)
{
	// TODO: Local sanity check on `Line`?
	struct sRichTextIPC_WriteLine	*msg;
	size_t	len = sizeof(*msg) + strlen(Text) + 1;
	char	buffer[len];
       	msg = (void*)buffer;
	msg->Line = Line;
	strcpy(msg->LineData, Text);
	AxWin3_SendIPC(Window, IPC_RICHTEXT_WRITELINE, len, msg);
}

