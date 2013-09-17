/*
 * Acess2 Kernel
 * - By John Hodge (thePowersGang)
 *
 * logdebug.h
 * - Logging / Debug printing functions
 */
#ifndef _KERNEL__LOGDEBUG_H_
#define _KERNEL__LOGDEBUG_H_

#include <stdarg.h>

// --- Logging ---
/**
 * \name Logging to kernel ring buffer
 * \{
 */
extern void	Log_KernelPanic(const char *Ident, const char *Message, ...);
extern void	Log_Panic(const char *Ident, const char *Message, ...);
extern void	Log_Error(const char *Ident, const char *Message, ...);
extern void	Log_Warning(const char *Ident, const char *Message, ...);
extern void	Log_Notice(const char *Ident, const char *Message, ...);
extern void	Log_Log(const char *Ident, const char *Message, ...);
extern void	Log_Debug(const char *Ident, const char *Message, ...);
/**
 * \}
 */

// --- Debug ---
/**
 * \name Debugging and Errors
 * \{
 */
extern void	Debug_KernelPanic(void);	//!< Initiate a kernel panic
extern void	Panic(const char *Msg, ...);	//!< Print a panic message (initiates a kernel panic)
extern void	Warning(const char *Msg, ...);	//!< Print a warning message
extern void	LogF(const char *Fmt, ...);	//!< Print a log message without a trailing newline
extern void	LogFV(const char *Fmt, va_list Args);	//!< va_list non-newline log message
extern void	Log(const char *Fmt, ...);	//!< Print a log message
extern void	Debug(const char *Fmt, ...);	//!< Print a debug message (doesn't go to KTerm)
extern void	LogV(const char *Fmt, va_list Args);	//!< va_list Log message
extern void	Debug_Enter(const char *FuncName, const char *ArgTypes, ...);
extern void	Debug_Log(const char *FuncName, const char *Fmt, ...);
extern void	Debug_Leave(const char *FuncName, char RetType, ...);
extern void	Debug_HexDump(const char *Header, const void *Data, size_t Length);
#define UNIMPLEMENTED()	Warning("'%s' unimplemented", __func__)
#if DEBUG
# define ENTER(_types...)	Debug_Enter((char*)__func__, _types)
# define LOG(_fmt...)	Debug_Log((char*)__func__, _fmt)
# define LEAVE(_t...)	Debug_Leave((char*)__func__, _t)
# define LEAVE_RET(_t,_v...)	do{LEAVE(_t,_v);return _v;}while(0)
# define LEAVE_RET0()	do{LEAVE('-');return;}while(0)
#else
# define ENTER(...)
# define LOG(...)
# define LEAVE(...)
# define LEAVE_RET(_t,_v...)	return (_v)
# define LEAVE_RET0()	return
#endif
#if !DISABLE_ASSERTS
# define ASSERTV(expr,msg,args...) do{if(!(expr))Panic("%s:%i - %s: Assertion '"#expr"' failed"msg,__FILE__,__LINE__,(char*)__func__,##args);}while(0)
# define ASSERTRV(expr,rv,msg,args...)	do{if(!(expr)){Warning("%s:%i: Assertion '"#expr"' failed"msg,__FILE__,__LINE__,(char*)__func__ , ##args);return rv;}}while(0)
#else
# define ASSERTV(expr)
# define ASSERTRV(expr)
#endif
#define assert(expr)	ASSERTV(expr, "")
#define ASSERT(expr)	ASSERTV(expr, "")
#define ASSERTR(expr,rv)	ASSERTRV(expr, rv, "")
#define ASSERTC(l,rel,r)	ASSERTV(l rel r, ": 0x%x"#rel"0x%x", l, r)
#define ASSERTCR(l,rel,r,rv)	ASSERTRV(l rel r, rv, ": 0x%x"#rel"0x%x", l, r)
/**
 * \}
 */

#endif

