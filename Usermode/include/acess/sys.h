/*
 * Acess2 System Interface Header
 */
#ifndef _SYS_SYS_H_
#define _SYS_SYS_H_

#include <stdint.h>

// === CONSTANTS ===
#define OPENFLAG_EXEC	0x01
#define OPENFLAG_READ	0x02
#define OPENFLAG_WRITE	0x04
#define	OPENFLAG_NOLINK	0x40
#ifndef SEEK_CUR
# define SEEK_SET	1
# define SEEK_CUR	0
# define SEEK_END	-1
#endif
#define CLONE_VM	0x10
#define GETMSG_IGNORE	((void*)-1)
#define FILEFLAG_DIRECTORY	0x10
#define FILEFLAG_SYMLINK	0x20

// === TYPES ===
struct s_sysACL {
	union {
		struct {
			unsigned	group: 1;
			unsigned	id:	31;
		};
		uint32_t	object;
	};
	union {
		struct {
			unsigned	invert: 1;
			unsigned	perms:	31;
		};
		uint32_t	rawperms;
	};
};
struct s_sysFInfo {
	uint	uid, gid;
	uint	flags;
	uint64_t	size;
	uint64_t	atime;
	uint64_t	mtime;
	uint64_t	ctime;
	 int	numacls;
	struct s_sysACL	acls[];
};
typedef struct s_sysFInfo	t_sysFInfo;
typedef struct s_sysACL	t_sysACL;

// === FUNCTIONS ===
extern void	_SysDebug(char *str, ...);
// --- Proc ---
extern void	_exit(int status)	__attribute__((noreturn));
extern void	sleep();
extern void	yield();
extern void	wait(int miliseconds);
extern int	waittid(int id, int *status);
extern int	clone(int flags, void *stack);
extern int	execve(char *path, char **argv, char **envp);
extern int	gettid();
extern int	getpid();

// --- Permissions ---
extern int	getuid();
extern int	getgid();
extern void	setuid(int id);
extern void	setgid(int id);

// --- VFS ---
extern int	chdir(char *dir);
extern int	open(char *path, int flags);
extern int	reopen(int fd, char *path, int flags);
extern void	close(int fd);
extern uint64_t	read(int fd, uint64_t length, void *buffer);
extern uint64_t	write(int fd, uint64_t length, void *buffer);
extern int	seek(int fd, uint64_t offset, int whence);
extern uint64_t	tell(int fd);
extern int	ioctl(int fd, int id, void *data);
extern int	finfo(int fd, t_sysFInfo *info, int maxacls);
extern int	readdir(int fd, char *dest);
extern int	_SysGetACL(int fd, t_sysACL *dest);
extern int	_SysMount(char *Device, char *Directory, char *Type, char *Options);

// --- IPC ---
extern int	SysSendMessage(int dest, int length, void *Data);
extern int	SysGetMessage(int *src, void *Data);

// --- MEMORY ---
uint64_t	_SysGetPhys(uint vaddr);
uint64_t	_SysAllocate(uint vaddr);

#endif
