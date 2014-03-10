
#ifndef _LIBACESSNATIVE_COMMON_H_
#define _LIBACESSNATIVE_COMMON_H_

extern int	giSyscall_ClientID;
extern void	Request_Preinit(void);
extern int	acess__SysOpen(const char *Path, unsigned int flags);
extern int	acessnative_spawn(const char *Binary, int SyscallID, const char * const * argv, const char * const * envp);

#define ENV_VAR_PREOPENS	"AN_PREOPEN"
#define ENV_VAR_KEY	"ACESSNATIVE_KEY"

#endif

