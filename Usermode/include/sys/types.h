/*
 */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H
#include <stdint.h>

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

#define	S_IFMT		0170000	/* type of file */
#define		S_IFDIR	0040000	/* directory */
#define		S_IFCHR	0020000	/* character special */
#define		S_IFBLK	0060000	/* block special */
#define		S_IFREG	0100000	/* regular */
#define		S_IFLNK	0120000	/* symbolic link */
#define		S_IFSOCK	0140000	/* socket */
#define		S_IFIFO	0010000	/* fifo */

#define FD_SETSIZE	128


#define CLONE_VM	0x10

/**
 * \brief fd_set for select()
 */
typedef struct
{
	uint16_t	flags[FD_SETSIZE/16];
}	fd_set;

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
	unsigned int	uid, gid;
	unsigned int	flags;
	uint64_t	size;
	uint64_t	atime;
	uint64_t	mtime;
	uint64_t	ctime;
	 int	numacls;
	struct s_sysACL	acls[];
};
typedef struct s_sysFInfo	t_sysFInfo;
typedef struct s_sysACL	t_sysACL;

static inline void FD_ZERO(fd_set *fdsetp) {int i=FD_SETSIZE/16;while(i--)fdsetp->flags[i]=0; }
static inline void FD_CLR(int fd, fd_set *fdsetp) { fdsetp->flags[fd/16]&=~(1<<(fd%16)); }
static inline void FD_SET(int fd, fd_set *fdsetp) { fdsetp->flags[fd/16]|=1<<(fd%16); }
static inline int FD_ISSET(int fd, fd_set *fdsetp) { return fdsetp->flags[fd/16]&(1<<(fd%16)); }

typedef uint32_t	pid_t;
typedef uint32_t	tid_t;
typedef  int64_t	time_t;

typedef unsigned int	uint;

#endif
