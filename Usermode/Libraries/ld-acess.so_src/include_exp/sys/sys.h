/*
 Syscall Definitions
*/
#ifndef _SYS_SYS_H_
#define _SYS_SYS_H_

#include <acess/sys.h>

#include <sys/types.h>

//#define O_RDONLY	OPENFLAG_READ
//#define O_WRONLY	OPENFLAG_WRITE
//#define O_CREAT 	(OPENFLAG_CREATE|OPENFLAG_WRITE)
//#define O_TRUNC 	OPENFLAG_WRITE
//#define O_APPEND 	OPENFLAG_WRITE


#if 0
#define	OPEN_FLAG_READ	1
#define	OPEN_FLAG_WRITE	2
#define	OPEN_FLAG_EXEC	4

enum {
	K_WAITPID_DIE = 0
};

// === System Calls ===
extern void	_exit(int ret);
extern int	brk(int bssend);
extern int	execve(char *file, char *args[], char *envp[]);
extern int	fork();
extern int	yield();
extern int	sleep();

extern int	open(char *file, int flags);
extern int	close(int fp);
extern int	read(int fp, int len, void *buf);
extern int	write(int fp, int len, void *buf);
extern int	tell(int fp);
extern void	seek(int fp, int64_t dist, int flag);
extern int	fstat(int fp, t_fstat *st);
extern int	ioctl(int fp, int call, void *arg);
extern int	readdir(int fp, char *file);

extern int	select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errfds, time_t *timeout);

extern int	kdebug(char *fmt, ...);
extern int	waitpid(int pid, int action);
extern int	gettid();	// Get Thread ID
extern int	getpid();	// Get Process ID
extern int	sendmsg(int dest, unsigned int *Data);
extern int	pollmsg(int *src, unsigned int *Data);
extern int	getmsg(int *src, unsigned int *Data);
#endif

#endif