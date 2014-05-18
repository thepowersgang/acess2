/*
 */
#include "window.h"
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>	// TODO: replace with calls into ACurses_*
#include <stdlib.h>
#include <assert.h>
#include "server.h"
#include <ctype.h>

struct sMessage
{
	struct sMessage	*Next;
	time_t	Timestamp;
	enum eMessageClass	Class;
	char	*Source;	// Pointer to the end of `Data`
	size_t	PrefixLen;
	char	Data[];
};

struct sWindow
{
	struct sWindow	*Next;
	tMessage	*Messages;
	tServer	*Server;	//!< Canonical server (can be NULL)
	 int	ActivityLevel;
	char	Name[];	// Channel name / remote user
};

// === PROTOTYPES ===
void	Windows_RepaintCurrent(void);
size_t	WordBreak(const char *Line, size_t avail);
size_t	Windows_int_PaintMessagePrefix(const tMessage *Message, bool EnablePrint);
size_t	Windows_int_PaintMessageLine(const tMessage *Message, size_t Offset, bool EnablePrint);
 int	Windows_int_GetMessageLines(const tMessage *Message);
 int	Windows_int_PaintMessage(tMessage *Message);

// === GLOBALS ===
tWindow	gWindow_Status = {
	NULL, NULL, NULL,	// No next, empty list, no server
	0, {"(status)"}	// No activity, empty name (rendered as status)
};
tWindow	*gpWindows = &gWindow_Status;
tWindow	*gpCurrentWindow = &gWindow_Status;

// === CODE ===
void Windows_RepaintCurrent(void)
{
	// TODO: Title bar?
	SetCursorPos(1, 1);
	printf("\x1b[37;44m\x1b[2K%s\x1b[0m\n", gpCurrentWindow->Name);

	 int	avail_rows = giTerminal_Height - 3;

	// Note: This renders from the bottom up
	tMessage *msg = gpCurrentWindow->Messages;
	for( int y = avail_rows; msg && y > 0; )
	{
		int lines = Windows_int_GetMessageLines(msg);
		y -= lines;
		size_t	ofs = 0;
		size_t	len;
		 int	i = 0;
		do {
			SetCursorPos(2 + y+i, 1);
			len = Windows_int_PaintMessageLine(msg, ofs, (y+i >= 0));
			ofs += len;
			i ++;
		} while( len > 0 );
		msg = msg->Next;
	}

	// Status line is our department
	SetCursorPos(giTerminal_Height-1, 1);
	printf("\x1b[37;44m\x1b[2K[%s] [%s/%s]\x1b[0m", Server_GetNick(gpCurrentWindow->Server),
		Server_GetName(gpCurrentWindow->Server), gpCurrentWindow->Name);
	fflush(stdout);
	// Bottom line is rendered by the prompt
}

size_t WordBreak(const char *Line, size_t avail)
{
	// If sufficient space, don't need to break on a word
	if( strlen(Line) < avail )
		return strlen(Line);
	
	// Search backwards from end of space for a non-alpha-numeric character
	size_t	ret = avail-1;
	while( ret > 0 && isalnum(Line[ret]) )
		ret --;
	
	// if one wasn't found in a sane area, just split
	if( ret < avail-20 || ret == 0 )
		return avail;
	
	return ret;
}

size_t Windows_int_PaintMessagePrefix(const tMessage *Message, bool EnablePrint)
{
	size_t	len = 0;
	
	unsigned long	seconds_today = Message->Timestamp/1000 % (24 * 3600);
	if(EnablePrint)
		printf("%02i:%02i:%02i ", seconds_today/3600, (seconds_today/60)%60, seconds_today%60);
	else
		len += snprintf(NULL, 0, "%02i:%02i:%02i ", seconds_today/3600, (seconds_today/60)%60, seconds_today%60);
	
	const char *format;
	switch(Message->Class)
	{
	case MSG_CLASS_BARE:
		format = "";
		break;
	case MSG_CLASS_CLIENT:
		if(Message->Source)
			format = "[%s] -!- ";
		else 
			format = "-!- ";
		break;
	case MSG_CLASS_WALL:
		format = "[%s] ";
		break;
	case MSG_CLASS_MESSAGE:
		format = "<%s> ";
		break;
	case MSG_CLASS_ACTION:
		format = "* %s ";
		break;
	}
	
	if( EnablePrint )
		len += printf(format, Message->Source);
	else
		len += snprintf(NULL, 0, format, Message->Source);

	return len;
}

size_t Windows_int_PaintMessageLine(const tMessage *Message, size_t Offset, bool EnablePrint)
{
	if( Message->Data[Offset] == '\0' ) {
		return 0;
	}
	
	size_t	avail = giTerminal_Width - Message->PrefixLen;
	const char *msg_data = Message->Data + Offset;
	int used = WordBreak(msg_data+Offset, avail);
	
	if( EnablePrint )
	{
		if( Offset == 0 )
			Windows_int_PaintMessagePrefix(Message, true);
		else
			printf("\x1b[%iC", Message->PrefixLen);
		printf("%.*s", used, msg_data);
	}
	
	if( msg_data[used] == '\0' )
		return 0;
	
	return Offset + used;
}

int Windows_int_GetMessageLines(const tMessage *Message)
{
	assert(Message->PrefixLen);
	const size_t	avail = giTerminal_Height - Message->PrefixLen;
	const size_t	msglen = strlen(Message->Data);
	size_t	offset = 0;
	 int	nLines = 0;
	do {
		offset += WordBreak(Message->Data+offset, avail);
		nLines ++;
	} while(offset < msglen);
	return nLines;
}

void Windows_SwitchTo(tWindow *Window)
{
	gpCurrentWindow = Window;
	Redraw_Screen();
}

void Windows_SetStatusServer(tServer *Server)
{
	gWindow_Status.Server = Server;
}

tWindow *Windows_GetByIndex(int Index)
{
	tWindow	*win;
	for( win = gpWindows; win && Index--; win = win->Next )
		;
	return win;
}

tWindow *Windows_GetByNameEx(tServer *Server, const char *Name, bool CreateAllowed)
{
	tWindow	*ret, *prev = NULL;
	 int	num = 1;
	
	// Get the end of the list and check for duplicates
	// TODO: Cache this instead
	for( ret = &gWindow_Status; ret; prev = ret, ret = ret->Next )
	{
		if( ret->Server == Server && strcmp(ret->Name, Name) == 0 )
		{
			return ret;
		}
		num ++;
	}
	if( !CreateAllowed ) {
		return NULL;
	}
	
	ret = malloc(sizeof(tWindow) + strlen(Name) + 1);
	ret->Messages = NULL;
	ret->Server = Server;
	ret->ActivityLevel = 1;
	strcpy(ret->Name, Name);
	
	if( prev ) {
		ret->Next = prev->Next;
		prev->Next = ret;
	}
	else {	// Shouldn't happen really
		ret->Next = gpWindows;
		gpWindows = ret;
	}
	
//	printf("Win %i %s:%s created\n", num, Server->Name, Name);
	
	return ret;
}

tWindow *Windows_GetByName(tServer *Server, const char *Name)
{
	return Windows_GetByNameEx(Server, Name, false);
}

tWindow *Window_Create(tServer *Server, const char *Name)
{
	return Windows_GetByNameEx(Server, Name, true);
}


tWindow *Window_int_ParseSpecial(const tWindow *Window)
{
	if( Window == NULL )
		return gpCurrentWindow;
	if( Window == WINDOW_STATUS )
		return &gWindow_Status;
	return (tWindow*)Window;
}

const char *Window_GetName(const tWindow *Window) {
	return Window_int_ParseSpecial(Window)->Name;
}
tServer	*Window_GetServer(const tWindow *Window) {
	return Window_int_ParseSpecial(Window)->Server;
}
bool Window_IsChat(const tWindow *Window) {
	return Window_int_ParseSpecial(Window) != &gWindow_Status;
}

// -----------------------------
//  Messages
// -----------------------------
void Window_AppendMessage(tWindow *Window, enum eMessageClass Class, const char *Source, const char *Message, ...)
{
	Window = Window_int_ParseSpecial(Window);

	va_list	args;
	
	va_start(args, Message);
	size_t len = vsnprintf(NULL, 0, Message, args);
	va_end(args);
	
	tMessage *msg = malloc( sizeof(tMessage) + len+1 + (Source?strlen(Source)+1:0) );
	assert(msg);
	
	msg->Class = Class;
	msg->Source = (Source ? msg->Data + len+1 : NULL);
	msg->Timestamp = _SysTimestamp();
	
	va_start(args, Message);
	vsnprintf(msg->Data, len+1, Message, args);
	va_end(args);
	
	if( Source ) {
		strcpy(msg->Source, Source);
	}
	
	msg->PrefixLen = Windows_int_PaintMessagePrefix(msg, false);
	
	msg->Next = Window->Messages;
	Window->Messages = msg;
	
	if( Window == gpCurrentWindow )
	{
		// Scroll if needed, and redraw?
		// - Lazy option of draw at bottom of screen
		printf("\33[s");	// Save cursor
		size_t	offset = 0, len;
		do {
			printf("\33[T");	// Scroll down 1 (free space below)
			SetCursorPos(giTerminal_Height-2, 1);
			len = Windows_int_PaintMessageLine(Message, offset, true);
			offset += len;
		} while( len > 0 );
		printf("\33[u");	// Restore cursor
		fflush(stdout);
	}
}

void Window_AppendMsg_Join(tWindow *Window, const char *Usermask)
{
	Window_AppendMessage(Window, MSG_CLASS_CLIENT, NULL, "%s has joined %s", Usermask, Window->Name);
}
void Window_AppendMsg_Quit(tWindow *Window, const char *Usermask, const char *Reason)
{
	Window_AppendMessage(Window, MSG_CLASS_CLIENT, NULL, "%s has joined quit (%s)", Usermask, Reason);
}
void Window_AppendMsg_Part(tWindow *Window, const char *Usermask, const char *Reason)
{
	Window_AppendMessage(Window, MSG_CLASS_CLIENT, NULL, "%s has joined left %s (%s)", Usermask, Window->Name, Reason);
}
void Window_AppendMsg_Topic(tWindow *Window, const char *Topic)
{
	Window_AppendMessage(Window, MSG_CLASS_CLIENT, NULL, "Topic of %s is %s", Window->Name, Topic);
}
void Window_AppendMsg_TopicTime(tWindow *Window, const char *User, const char *Timestamp)
{
	Window_AppendMessage(Window, MSG_CLASS_CLIENT, NULL, "Topic set by %s at %s", User, Timestamp);
}
void Window_AppendMsg_Kick(tWindow *Window, const char *Operator, const char *Nick, const char *Reason)
{
	Window_AppendMessage(Window, MSG_CLASS_CLIENT, NULL, "%s was kicked from %s by %s [%s]",
		Nick, Window->Name, Operator, Reason);
}
void Window_AppendMsg_Mode(tWindow *Window, const char *Operator, const char *Flags, const char *Args)
{
	Window_AppendMessage(Window, MSG_CLASS_CLIENT, NULL, "mode/%s [%s %s] by %s",
		Window->Name, Flags, Args, Operator);
}

