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
#define SEEK_SET	1
#define SEEK_CUR	0
#define SEEK_END	-1
#define CLONE_VM	0x10
#define FILEFLAG_DIRECTORY	0x10
#define FILEFLAG_SYMLINK	0x20

// === TYPES ===
struct s_sysACL {
	uint32_t	object;
	uint32_t	perms;
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

// === FUNCTIONS ===
void	_SysDebug(char *str, ...);
// --- Proc ---
void	sleep();
 int	clone(int flags, void *stack);
 int	execve(char *path, char **argv, char **envp);
// --- VFS ---
 int	open(char *path, int flags);
 int	reopen(int fd, char *path, int flags);
void	close(int fd);
uint64_t	read(int fd, uint64_t length, void *buffer);
uint64_t	write(int fd, uint64_t length, void *buffer);
 int	seek(int fd, uint64_t offset, int whence);
 int	ioctl(int fd, int id, void *data);
 int	finfo(int fd, t_sysFInfo *info, int maxacls);

// --- MEMORY ---
uint64_t	_SysGetPhys(uint vaddr);
uint64_t	_SysAllocate(uint vaddr);

#endif
