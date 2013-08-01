/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * grp.h
 * - Group Management
 */
#ifndef _LIBPOSIX__GRP_H_
#define _LIBPOSIX__GRP_H_

#include <sys/types.h>

struct group
{
	char	*gr_name;
	char	*gr_password;
	gid_t	gr_gid;
	char	**gr_mem;	// Members
};

extern int	initgroups(const char *user, gid_t group);
extern struct group *getgrnam(const char *name);
extern struct group *getgrgid(gid_t gid);
extern int	getgrnam_r(const char *name, struct group *grp, char *buf, size_t buflen, struct group **result);
extern int	getgrgid_r(gid_t gid, struct group *grp, char *buf, size_t buflen, struct group **result);

#endif

