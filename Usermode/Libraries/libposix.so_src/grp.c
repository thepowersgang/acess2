/*
 * Acess2 POSIX Emulation Layer
 * - By John Hodge
 * 
 * grp.c
 * - Group management
 */
#include <grp.h>
#include <string.h>
#include <errno.h>

// === GLOBALS ===
char	*group_wheel_members[] = {"root", NULL};
struct group	group_wheel = {
	.gr_name = "wheel",
	.gr_password = "",
	.gr_gid = 0,
	.gr_mem = group_wheel_members
};

// === CODE ===
int initgroups(const char *user, gid_t group)
{
	if( strcmp(user, "root") != 0 ) {
		errno = EINVAL;
		return 1;
	}
	
	return 0;
}
struct group *getgrnam(const char *name)
{
	if( strcmp(name, group_wheel.gr_name) == 0 )
	{
		return &group_wheel;
	}
	return NULL;
}

struct group *getgrgid(gid_t gid)
{
	if( gid == 0 )
		return &group_wheel;
	return NULL;
}

//extern int	getgrnam_r(const char *name, struct group *grp, char *buf, size_t buflen, struct group **result);
//extern int	getgrgid_r(gid_t gid, struct group *grp, char *buf, size_t buflen, struct group **result);


