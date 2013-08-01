/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge
 * 
 * unistd.h
 * - 
 */
#ifndef _UNISTD_H_
#define _UNISTD_H_

#include <stddef.h>

//! \brief flags for open(2)
#define O_WRONLY	0x01
#define O_RDONLY	0x02
#define	O_RDWR  	0x03
#define O_APPEND	0x04
#define O_CREAT 	0x08
#define O_DIRECTORY	0x10
#define O_ASYNC 	0x20
#define O_TRUNC 	0x40
#define O_NOFOLLOW	0x80	// don't follow symlinks
#define	O_EXCL  	0x100
#define O_NOCTTY	0	// unsupported
#define O_NONBLOCK	0x200
#define	O_SYNC	0	// not supported

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

typedef signed long	ssize_t;

#include "sys/stat.h"	// mode_t

extern int	open(const char *path, int flags, ...);
extern int	creat(const char *path, mode_t mode);
extern int	close(int fd);

extern ssize_t	write(int fd, const void *buf, size_t count);
extern ssize_t	read(int fd, void *buf, size_t count);
extern off_t	lseek(int fd, off_t offset, int whence);

extern int	fork(void);
extern int	execv(const char *b, char *v[]);

extern int	dup2(int oldfd, int newfd);

extern int	chown(const char *path, uid_t owner, gid_t group);

#define S_ISUID	04000
#define S_ISGID	02000
#define S_ISVTX	01000
#define S_IRWXU	00700
#define S_IRUSR	00400
#define S_IWUSR	00300
#define S_IXUSR	00100
#define S_IRWXG	00070
#define S_IRGRP	00040
#define S_IWGRP	00020
#define S_IXGRP	00010
#define S_IRWXO	0007
#define S_IROTH	00004
#define S_IWOTH	00002
#define S_IXOTH	00001
extern int	chmod(const char *path, mode_t mode);

extern pid_t	setsid(void);

extern uid_t	getuid(void);
//extern int	setuid(uid_t uid);
extern uid_t	geteuid(void);
extern pid_t	getpid(void);
extern int	seteuid(uid_t euid);
extern int	setegid(gid_t egid);
//extern int	setgid(gid_t gid);

typedef uint32_t	useconds_t;

extern unsigned int	sleep(unsigned int seconds);
extern int	usleep(useconds_t usec);

// - crypt.c
extern char	*crypt(const char *key, const char *salt);

// - pty.c
extern char	*ttyname(int fd);
extern int	ttyname_r(int fd, char *buf, size_t buflen);

// signal.h / sys/types.h
extern int kill(pid_t pid, int sig);

extern int	chdir(const char *dir);
extern int	rmdir(const char *pathname);

// Deprecated POSIX.1-2001
#define PASS_MAX	63
extern char *getpass(const char *prompt);

#endif

