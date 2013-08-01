/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * sys/wait.h
 * - Waiting
 */
#ifndef _LIBPOSIX__SYS__WAIT_H_
#define _LIBPOSIX__SYS__WAIT_H_

#include <sys/types.h>

// POSIX, waitpid()
#define	WNOHANG	0x01
#define WUNTRACED	0x02

// POSIX, status values
#define WIFEXITED(v)	(((v)>>16)==0)
#define WEXITSTATUS(v)	(v&0xFF)
#define WIFSIGNALED(v)	(((v)>>16)==1)
#define WTERMSIG(v)	(v&0xFFF)
#define WCOREDUMP(v)	(!!(v>>12))
#define WIFSTOPPED(v)	0
#define WSTOPSIG(v)	0
#define WIFCONTINUED(v)	0

// POSIX/XSI, waitid(options)
#define WEXITED 	0x10
#define WSTOPPED	0x20
#define WCONTINUED	0x40
#define WNOWAIT 	0x80

// POSIX/XSI, idtype_t
typedef enum
{
	P_ALL,
	P_PID,
	P_PGID
} idtype_t;

// POSIX
extern pid_t	wait(int *status);
// POSIX/XSI
//extern int	waitid(idtype_t, id_t, siginfo_t *, int);
// POSIX
extern pid_t	waitpid(pid_t pid, int *status, int options);


#endif

