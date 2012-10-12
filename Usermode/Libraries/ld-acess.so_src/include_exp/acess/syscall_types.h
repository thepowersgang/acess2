/*
 * Acess2 Dynamic Linker
 * - By John Hodge (thePowersGang)
 *
 * acess/syscall_types.h
 * - Structures used for syscalls
 */

#ifndef _ACESS__SYSCALL_TYPES_H_
#define _ACESS__SYSCALL_TYPES_H_

struct s_sysACL {
	unsigned long	object;	/*!< Group or user (bit 31 determines) */
	unsigned long	perms;	/*!< Inverted by bit 31 */
};
struct s_sysFInfo {
	unsigned int	mount;
	unsigned long long	inode;
	unsigned int	uid;
	unsigned int	gid;
	unsigned int	flags;
	unsigned long long	size;
	time_t	atime;
	time_t	mtime;
	time_t	ctime;
	 int	numacls;
	struct s_sysACL	acls[];
} __attribute__((packed));
typedef struct s_sysFInfo	t_sysFInfo;
typedef struct s_sysACL	t_sysACL;

struct s_sys_spawninfo
{
	unsigned int	flags;
	unsigned int	uid;
	unsigned int	gid;
};

#endif

