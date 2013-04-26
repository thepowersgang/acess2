/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * pwd.h
 * - Password Structure
 */
#ifndef _LIBPOSIX__PWD_H_
#define _LIBPOSIX__PWD_H_

#include <sys/types.h>	// gid_t/uid_t

struct passwd
{
	char	*pw_name;
	uid_t	pw_uid;
	gid_t	pw_gid;
	char	*pw_dir;
	char	*pw_shell;
	char	*pw_passwd;
};

extern struct passwd	*getpwnam(const char *);
extern struct passwd	*getpwuid(uid_t);
extern int	getpwnam_r(const char *, struct passwd *, char *, size_t, struct passwd **);
extern int	getpwuid_r(uid_t, struct passwd *, char *, size_t, struct passwd **);
extern void	endpwent(void);
extern struct passwd	*getpwent(void);
extern void	setpwent(void);

#endif

