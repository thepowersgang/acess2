
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
# define LEAVE_RET(t,v)	return v;
#endif

extern void	Log_KernelPanic(const char *Ident, const char *Message, ...) __attribute__((noreturn));
extern void	Log_Panic(const char *Ident, const char *Message, ...);
extern void	Log_Error(const char *Ident, const char *Message, ...);
extern void	Log_Warning(const char *Ident, const char *Message, ...);
extern void	Log_Notice(const char *Ident, const char *Message, ...);
extern void	Log_Log(const char *Ident, const char *Message, ...);
extern void	Log_Debug(const char *Ident, const char *Message, ...);

extern void	Panic(const char *Msg, ...);
extern void	Warning(const char *Message, ...);
extern void	Log(const char *Message, ...);
extern void	Debug_HexDump(const char *Prefix, const void *Data, size_t Length);

extern void	Debug_TraceEnter(const char *Function, const char *Format, ...);
extern void	Debug_TraceLog(const char *Function, const char *Format, ...);
extern void	Debug_TraceLeave(const char *Function, char Type, ...);


#if !DISABLE_ASSERTS
# define ASSERTV(expr,msg,args...) do{if(!(expr))Panic("%s:%i - %s: Assertion '"#expr"' failed"msg,__FILE__,__LINE__,(char*)__func__,##args);}while(0)
# define ASSERTRV(expr,rv,msg,args...)	do{if(!(expr)){Warning("%s:%i: Assertion '"#expr"' failed"msg,__FILE__,__LINE__,(char*)__func__ , ##args);return rv;}}while(0)
#else
# define ASSERTV(expr)
# define ASSERTRV(expr)
#endif
#define ASSERT(expr)	ASSERTV(expr, "")
#define ASSERTR(expr,rv)	ASSERTRV(expr, rv, "")
#define ASSERTC(l,rel,r)	ASSERTV(l rel r, ": %i"#rel"%i", l, r)
#define ASSERTCR(l,rel,r,rv)	ASSERTRV(l rel r, rv, ": %i"#rel"%i", l, r)

#endif

