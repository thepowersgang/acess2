/*
 * Acess2 System Interface Header
 */
#ifndef _ACESS_SYS_H_
#define _ACESS_SYS_H_

#include <stdint.h>
#include "../sys/types.h"

// === CONSTANTS ===
#ifndef NULL
# define NULL	((void*)0)
#endif

#define OPENFLAG_EXEC	0x01
#define OPENFLAG_READ	0x02
#define OPENFLAG_WRITE	0x04
#define	OPENFLAG_NOLINK	0x40
#define	OPENFLAG_CREATE	0x80
#ifndef SEEK_CUR
# define SEEK_SET	1
# define SEEK_CUR	0
# define SEEK_END	-1
#endif
#define GETMSG_IGNORE	((void*)-1)
#define FILEFLAG_DIRECTORY	0x10
#define FILEFLAG_SYMLINK	0x20

// === TYPES ===

// === VARIABLES ===
extern int	_errno;

// === FUNCTIONS ===
extern void	_SysDebug(const char *format, ...);
// --- Proc ---
extern void	_exit(int status)	__attribute__((noreturn));
extern void	sleep(void);
extern void	yield(void);
extern int	kill(int pid, int sig);
extern void	wait(int miliseconds);
extern int	waittid(int id, int *status);
extern int	clone(int flags, void *stack);
extern int	execve(char *path, char **argv, char **envp);
extern int	gettid(void);
extern int	getpid(void);
extern int	_SysSetFaultHandler(int (*Handler)(int));
extern void	SysSetName(const char *Name);
//extern int	SysGetName(const char *Name);

// --- Permissions ---
extern int	getuid(void);
extern int	getgid(void);
extern void	setuid(int id);
extern void	setgid(int id);

// --- VFS ---
extern int	chdir(const char *dir);
extern int	open(const char *path, int flags);
extern int	reopen(int fd, const char *path, int flags);
extern int	close(int fd);
extern uint	read(int fd, void *buffer, uint length);
extern uint	write(int fd, const void *buffer, uint length);
extern int	seek(int fd, int64_t offset, int whence);
extern uint64_t	tell(int fd);
extern int	ioctl(int fd, int id, void *data);
extern int	finfo(int fd, t_sysFInfo *info, int maxacls);
extern int	readdir(int fd, char *dest);
extern int	_SysOpenChild(int fd, char *name, int flags);
extern int	_SysGetACL(int fd, t_sysACL *dest);
extern int	_SysMount(const char *Device, const char *Directory, const char *Type, const char *Options);
extern int	select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errfds, time_t *timeout);

// --- IPC ---
extern int	SysSendMessage(pid_t dest, uint length, const void *Data);
extern int	SysGetMessage(pid_t *src, void *Data);

// --- MEMORY ---
uint64_t	_SysGetPhys(uint vaddr);
uint64_t	_SysAllocate(uint vaddr);

#endif
