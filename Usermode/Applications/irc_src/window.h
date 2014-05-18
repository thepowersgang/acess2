/*
 */
#ifndef _WINDOW_H_
#define _WINDOW_H_

#include "common.h"
#include "message.h"
#include <stdbool.h>

typedef struct sWindow	tWindow;
extern void	Windows_RepaintCurrent(void);

extern void	Windows_SetStatusServer(tServer *Server);
extern tWindow	*Window_Create(tServer *Server, const char *Name);
extern tWindow	*Windows_GetByIndex(int Index);
extern tWindow	*Windows_GetByName(tServer *Server, const char *Name);
static inline tWindow	*Windows_GetByNameOrCreate(tServer *Server, const char *Name) {
	return Window_Create(Server, Name);
}
extern void	Windows_SwitchTo(tWindow *Window);

extern void	Window_AppendMessage(tWindow *Window, enum eMessageClass Class, const char *Source, const char *Message, ...)
	__attribute__((format(__printf__,4,5)));
extern void	Window_AppendMsg_Join(tWindow *Window, const char *Usermask);
extern void	Window_AppendMsg_Quit(tWindow *Window, const char *Usermask, const char *Reason);
extern void	Window_AppendMsg_Part(tWindow *Window, const char *Usermask, const char *Reason);
extern void	Window_AppendMsg_Kick(tWindow *Window, const char *Operator, const char *Nick, const char *Reason);
extern void	Window_AppendMsg_Mode(tWindow *Window, const char *Operator, const char *Flags, const char *Args);
extern void	Window_AppendMsg_Topic(tWindow *Window, const char *Topic);
extern void	Window_AppendMsg_TopicTime(tWindow *Window, const char *User, const char *Timestmap);

extern const char	*Window_GetName(const tWindow *Window);
extern tServer	*Window_GetServer(const tWindow *Window);
extern bool	Window_IsChat(const tWindow *Window);

#define WINDOW_STATUS	((void*)-1)

#endif
