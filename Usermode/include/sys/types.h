/*
 */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H
//#include <stdint.h>

typedef struct {
	int		st_dev;		//dev_t
	int		st_ino;		//ino_t
	int		st_mode;	//mode_t
	unsigned int	st_nlink;
	unsigned int	st_uid;
	unsigned int	st_gid;
	int		st_rdev;	//dev_t
	unsigned int	st_size;
	long	st_atime;	//time_t
	long	st_mtime;
	long	st_ctime;
} t_fstat;

#define FD_SETSIZE	128


#define CLONE_VM	0x10

typedef unsigned long	pid_t;
typedef unsigned long	tid_t;
typedef signed long long	time_t;

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
	unsigned long	object;	//!< Group or user (bit 31 determines)
	unsigned long	perms;	//!< Inverted by bit 31
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
};
typedef struct s_sysFInfo	t_sysFInfo;
typedef struct s_sysACL	t_sysACL;

static inline void FD_ZERO(fd_set *fdsetp) {int i=FD_SETSIZE/16;while(i--)fdsetp->flags[i]=0; }
static inline void FD_CLR(int fd, fd_set *fdsetp) {
	if(fd < 0 || fd > FD_SETSIZE)	return;
	fdsetp->flags[fd/16] &= (fd_set_ent_t) ((~1 << (fd%16))) & 0xFFFF;
}
static inline void FD_SET(int fd, fd_set *fdsetp) {
	if(fd < 0 || fd > FD_SETSIZE)	return;
	fdsetp->flags[fd/16] |= (fd_set_ent_t) (1 << (fd%16));
}
static inline int FD_ISSET(int fd, fd_set *fdsetp) {
	if(fd < 0 || fd > FD_SETSIZE)	return 0;
	return !!( fdsetp->flags[fd/16] & (1<<(fd%16)) );
}

#endif
