/*
 */
#include "window.h"
#include <stddef.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>	// TODO: replace with calls into ACurses_*
#include <stdlib.h>
#include <assert.h>

struct sMessage
{
	struct sMessage	*Next;
	time_t	Timestamp;
	enum eMessageClass	Class;
	char	*Source;	// Pointer to the end of `Data`
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
 int	Windows_int_PaintMessage(tMessage *Message);

// === GLOBALS ===
tWindow	gWindow_Status = {
	NULL, NULL, NULL,	// No next, empty list, no server
	0, {""}	// No activity, empty name (rendered as status)
};
tWindow	*gpWindows = &gWindow_Status;
tWindow	*gpCurrentWindow = &gWindow_Status;

// === CODE ===
void Windows_RepaintCurrent(void)
{
	tMessage *msg = gpCurrentWindow->Messages;
	
	// TODO: Title bar?

	// Note: This renders from the bottom up
	for( int y = giTerminal_Height - 1; y -- && msg; msg = msg->Next)
	{
		y -= Windows_int_PaintMessage(msg);
	}

	// Bottom line is rendered by the prompt
	
}

int Windows_int_PaintMessage(tMessage *Message)
{
	printf("\33[T");	// Scroll down 1 (free space below)
	SetCursorPos(giTerminal_Height-2, 1);
	
	size_t	prefixlen = 0;
	prefixlen += printf("%02i:%02i:%02i ", (Message->Timestamp/3600)%24, (Message->Timestamp/60)%60, Message->Timestamp%60);
	switch(Message->Class)
	{
	case MSG_CLASS_BARE:	break;
	case MSG_CLASS_CLIENT:
		if(Message->Source)
			prefixlen += printf("[%s] ", Message->Source);
		prefixlen += printf("-!- ");
		break;
	case MSG_CLASS_WALL:
		prefixlen += printf("[%s] ", Message->Source);
		break;
	case MSG_CLASS_MESSAGE:
		prefixlen += printf("<%s> ", Message->Source);
		break;
	case MSG_CLASS_ACTION:
		prefixlen += printf("* %s ", Message->Source);
		break;
	}
	 int	avail = giTerminal_Width - prefixlen;
	 int	msglen = strlen(Message->Data);
	
	int	nLines = 1;
	printf("%.*s", avail, Message);
	while( msglen > avail ) {
		msglen -= avail;
		Message += avail;
		printf("\33[T");
		SetCursorPos(giTerminal_Height-2, prefixlen+1);
		printf("%.*s", avail, Message);
		nLines ++;
	}
	
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
	va_start(args, Message);
	vsnprintf(msg->Data, len+1, Message, args);
	va_end(args);
	
	msg->Next = Window->Messages;
	Window->Messages = msg;
	
	if( Window == gpCurrentWindow )
	{
		// Scroll if needed, and redraw?
		// - Lazy option of draw at bottom of screen
		printf("\33[s");	// Save cursor
		Windows_int_PaintMessage(msg);
		printf("\x1b[u");	// Restore cursor
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

