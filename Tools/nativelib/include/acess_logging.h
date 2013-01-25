
#ifndef _DISKTOOL__ACESS_LOGGING_H_
#define _DISKTOOL__ACESS_LOGGING_H_

#if DEBUG
# define ENTER(str, v...)	Debug_TraceEnter(__func__, str, ##v)
# define LOG(fmt, v...) 	Debug_TraceLog(__func__, fmt, ##v)
# define LEAVE(t, v...)     	Debug_TraceLeave(__func__, t, ##v)
# define LEAVE_RET(t,v)	do{LEAVE('-');return v;}while(0)
#else
# define ENTER(...)	do{}while(0)
# define LOG(...)	do{}while(0)
# define LEAVE(...)	do{}while(0)
# define LEAVE_RET(t,v)	do{return v;}while(0)
#endif

extern void	Log_KernelPanic(const char *Ident, const char *Message, ...) __attribute__((noreturn));
extern void	Log_Panic(const char *Ident, const char *Message, ...);
extern void	Log_Error(const char *Ident, const char *Message, ...);
extern void	Log_Warning(const char *Ident, const char *Message, ...);
extern void	Log_Notice(const char *Ident, const char *Message, ...);
extern void	Log_Log(const char *Ident, const char *Message, ...);
extern void	Log_Debug(const char *Ident, const char *Message, ...);

extern void	Warning(const char *Message, ...);
extern void	Log(const char *Message, ...);
extern void	Debug_HexDump(const char *Prefix, const void *Data, size_t Length);

extern void	Debug_TraceEnter(const char *Function, const char *Format, ...);
extern void	Debug_TraceLog(const char *Function, const char *Format, ...);
extern void	Debug_TraceLeave(const char *Function, char Type, ...);

#endif

