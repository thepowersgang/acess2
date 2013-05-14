/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * utmpx.h
 * - User login records
 */
#ifndef _LIBPOSIX__UTMPX_H_
#define _LIBPOSIX__UTMPX_H_

#include <sys/time.h>	// struct timeval

#define UTMPX_FILE	"/Acess/var/utmpx"

enum e_utmpx_type_vals
{
	EMPTY,	//!< No information
	BOOT_TIME,	//!< Identifies time of system boot
	OLD_TIME,	//!< Old time when system time was changed
	NEW_TIME,	//!< New time when system time was changed
	USER_PROCESS,	//!< Any process
	INIT_PROCESS,	//!< Process spawned by init
	LOGIN_PROCESS,	//!< Session leader for a logged-in user
	DEAD_PROCESS	//!< Exited session leader
};

#define UT_USER_MAX	32
#define UT_ID_MAX	8
#define UT_LINE_MAX	32

struct utmpx
{
	char	ut_user[UT_USER_MAX];
	char	ut_id[UT_ID_MAX];
	char	ut_line[UT_LINE_MAX];
	pid_t	ut_pid;
	short	ut_type;
	struct timeval	ut_tv;
};

extern void	endutxent(void);
extern struct utmpx	*getutxent(void);
extern struct utmpx	*getutxid(const struct utmpx *);
extern struct utmpx	*getutxline(const struct utmpx *);
extern struct utmpx	*pututxline(const struct utmpx *);
extern void	setutxent(void);

#endif

