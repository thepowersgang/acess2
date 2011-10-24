/*
 */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

typedef struct stat	t_fstat;

#define FD_SETSIZE	128


#define CLONE_VM	0x10

typedef unsigned long	pid_t;
typedef unsigned long	tid_t;
typedef signed long long int	time_t;
typedef long long int	off_t;

typedef unsigned int	uint;

typedef unsigned short	fd_set_ent_t;

/**
 * \brief fd_set for select()
 */
typedef struct
{
	fd_set_ent_t	flags[FD_SETSIZE/16];
}	fd_set;

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

extern void	FD_ZERO(fd_set *fdsetp);
extern void	FD_CLR(int fd, fd_set *fdsetp);
extern void	FD_SET(int fd, fd_set *fdsetp);
extern int	FD_ISSET(int fd, fd_set *fdsetp);

#include "../sys/stat.h"

#endif
