/*
 * Acess2 POSIX Emulation Lib
 * - By John Hodge (thePowersGang)
 *
 * sys/stat.h
 * - stat(2)
 */
#ifndef _SYS_STAT_H_
#define _SYS_STAT_H_

#include <stdint.h>
#include "sys/types.h"	// off_t

#ifdef __cplusplus
extern "C" {
#endif

typedef void	*dev_t;	/* TODO: How to identify a device with Acess */
typedef uint64_t	ino_t;
typedef unsigned int	blksize_t;
typedef uint64_t	blkcnt_t;
typedef unsigned int	nlink_t;
typedef uint32_t	mode_t;

#define	S_IFMT		0170000	/* type of file */
#define		S_IFDIR	0040000	/* directory */
#define		S_IFCHR	0020000	/* character special */
#define		S_IFBLK	0060000	/* block special */
#define		S_IFREG	0100000	/* regular */
#define		S_IFLNK	0120000	/* symbolic link */
#define		S_IFSOCK	0140000	/* socket */
#define		S_IFIFO	0010000	/* fifo */

#define S_ISDIR(mode)	(((mode) & S_IFMT) == S_IFDIR)
#define S_ISREG(mode)	(((mode) & S_IFMT) == S_IFREG)
#define S_ISCHR(mode)	(((mode) & S_IFMT) == S_IFCHR)

struct stat
{
	dev_t     st_dev;     /* ID of device containing file */
	ino_t     st_ino;     /* inode number */
	mode_t    st_mode;    /* protection */
	nlink_t   st_nlink;   /* number of hard links */
	uid_t     st_uid;     /* user ID of owner */
	gid_t     st_gid;     /* group ID of owner */
	dev_t     st_rdev;    /* device ID (if special file) */
	off_t     st_size;    /* total size, in bytes */
	blksize_t st_blksize; /* blocksize for file system I/O */
	blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
	time_t    st_atime;   /* time of last access */
	time_t    st_mtime;   /* time of last modification */
	time_t    st_ctime;   /* time of last status change */
};

extern int stat(const char *path, struct stat *buf);
extern int lstat(const char *path, struct stat *buf);
extern int fstat(int fd, struct stat *buf);
extern int mkdir(const char *pathname, mode_t mode);

#ifdef __cplusplus
}
#endif

#endif
