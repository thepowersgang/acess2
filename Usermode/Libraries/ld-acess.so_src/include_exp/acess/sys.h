/*
 * Acess2 System Interface Header
 */
#ifndef _ACESS_SYS_H_
#define _ACESS_SYS_H_

#include <stdint.h>
#include <stddef.h>	// size_t
#include "syscall_types.h"

// === CONSTANTS ===
#ifndef NULL
# define NULL	((void*)0)
#endif

#define THREAD_EVENT_VFS	0x0001
#define THREAD_EVENT_IPCMSG	0x0002
#define THREAD_EVENT_SIGNAL	0x0004

#define OPENFLAG_EXEC	0x01
#define OPENFLAG_READ	0x02
#define OPENFLAG_WRITE	0x04
#define OPENFLAG_RDWR	(OPENFLAG_READ|OPENFLAG_WRITE)
#define OPENFLAG_TRUNCATE	0x10
#define OPENFLAG_APPEND	0x20
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
#define CLONE_VM	0x10

#ifdef ARCHDIR_is_native
# include "_native_syscallmod.h"
#endif

// === TYPES ===

// === VARIABLES ===
extern int	_errno;

// === FUNCTIONS ===
extern void	_SysDebug(const char *format, ...);
// --- Proc ---
extern void	_exit(int status)	__attribute__((noreturn));
extern int	_SysKill(int pid, int sig);
extern int	_SysWaitEvent(int EventMask);
extern int	_SysWaitTID(int id, int *status);
extern int	_SysClone(int flags, void *stack);
extern int	_SysExecVE(const char *path, char **argv, char **envp);
extern int	_SysSpawn(const char *Path, const char **argv, const char **envp, int nFDs, int *FDs, struct s_sys_spawninfo *info);
extern int	gettid(void);
extern int	_SysGetPID(void);
extern int	_SysSetFaultHandler(int (*Handler)(int));
extern void	_SysSetName(const char *Name);
extern int	_SysGetName(char *NameDest);
extern int	_SysSetPri(int Priority);
// --- Timekeeping ---
extern int64_t	_SysTimestamp(void);

// --- Permissions ---
extern int	_SysGetUID(void);
extern int	_SysGetGID(void);
extern void	setuid(int id);
extern void	setgid(int id);

// --- VFS ---
extern int	_SysChdir(const char *dir);
extern int	_SysChroot(const char *dir);

extern int	_SysOpen(const char *path, int flags, ...);
extern int	_SysOpenChild(int fd, const char *name, int flags);
extern int	_SysOpenPipe(int *read, int *write, int flags);
extern int	_SysReopen(int fd, const char *path, int flags);
extern int	_SysCopyFD(int srcfd, int dstfd);
extern int	_SysFDFlags(int fd, int mask, int newflags);
extern size_t	_SysRead(int fd, void *buffer, size_t length);
extern int	_SysClose(int fd);
extern int	_SysFDCtl(int fd, int option, ...);
extern size_t	_SysWrite(int fd, const void *buffer, size_t length);
extern int	_SysSeek(int fd, int64_t offset, int whence);
extern uint64_t	_SysTell(int fd);
extern int	_SysIOCtl(int fd, int id, void *data);
extern int	_SysFInfo(int fd, t_sysFInfo *info, int maxacls);
extern int	_SysReadDir(int fd, char *dest);
extern int	_SysGetACL(int fd, t_sysACL *dest);
extern int	_SysMount(const char *Device, const char *Directory, const char *Type, const char *Options);
extern int	_SysSelect(int nfds, fd_set *read, fd_set *write, fd_set *err, int64_t *timeout, unsigned int extraevents);
//#define select(nfs, rdfds, wrfds, erfds, timeout)	_SysSelect(nfs, rdfds, wrfds, erfds, timeout, 0)
extern int	_SysUnlink(const char *pathname);

// --- IPC ---
extern int	_SysSendMessage(int dest, size_t length, const void *Data);
extern int	_SysGetMessage(int *src, size_t buflen, void *Data);

// --- MEMORY ---
extern uint64_t	_SysGetPhys(uintptr_t vaddr);
extern uint64_t	_SysAllocate(uintptr_t vaddr);
extern uint32_t	_SysSetMemFlags(uintptr_t vaddr, uint32_t flags, uint32_t mask);
extern void	*_SysLoadBin(const char *path, void **entry);
extern int	_SysUnloadBin(void *base);
extern void	SysSetFaultHandler(int (*Hanlder)(int));

#endif
