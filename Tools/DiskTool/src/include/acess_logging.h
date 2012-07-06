
#ifndef _DISKTOOL__ACESS_LOGGING_H_
#define _DISKTOOL__ACESS_LOGGING_H_

extern void	Log_KernelPanic(const char *Ident, const char *Message, ...) __attribute__((noreturn));
extern void	Log_Panic(const char *Ident, const char *Message, ...);
extern void	Log_Error(const char *Ident, const char *Message, ...);
extern void	Log_Warning(const char *Ident, const char *Message, ...);
extern void	Log_Notice(const char *Ident, const char *Message, ...);
extern void	Log_Log(const char *Ident, const char *Message, ...);
extern void	Log_Debug(const char *Ident, const char *Message, ...);

extern void	Warning(const char *Message, ...);
extern void	Log(const char *Message, ...);
#endif

