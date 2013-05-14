/*
 * Acess2 POSIX Emulation
 * - By John Hodge (thePowersGang)
 *
 * pwd.c
 * - Password Structure
 */
#include <pwd.h>
#include <errno.h>
#include <string.h>

// TODO: Parse something like '/Acess/Conf/Users'

// === GLOBALS ===
static const struct passwd	gPwd_RootInfo = {
	.pw_name = "root",
	.pw_uid = 0,
	.pw_gid = 0,
	.pw_dir = "/Acess/Root",
	.pw_shell = "/Acess/Bin/CLIShell",
	.pw_passwd = "",
};

// === CODE ===
struct passwd *getpwnam(const char *name)
{
	static struct passwd	ret_struct;
	static char	ret_buf[64];
	struct passwd	*ret_ptr;
	errno = getpwnam_r(name, &ret_struct, ret_buf, sizeof(ret_buf), &ret_ptr);
	return ret_ptr;
}

struct passwd *getpwuid(uid_t uid)
{
	static struct passwd	ret_struct;
	static char	ret_buf[64];
	struct passwd	*ret_ptr;
	errno = getpwuid_r(uid, &ret_struct, ret_buf, sizeof(ret_buf), &ret_ptr);
	return ret_ptr;
}

static int fill_pwd(const struct passwd *tpl, struct passwd *pwd, char *buf, size_t buflen)
{
	size_t	ofs = 0;
	#define _setstr(field)	do { \
		if( ofs + strlen(tpl->field)+1 > buflen ) \
			return ERANGE; \
		pwd->field = buf + ofs; \
		strcpy(pwd->field, tpl->field); \
	} while(0)
	_setstr(pw_name);
	pwd->pw_uid = tpl->pw_uid;
	pwd->pw_gid = tpl->pw_gid;
	_setstr(pw_dir);
	_setstr(pw_shell);
	_setstr(pw_passwd);
	return 0;
}

int getpwnam_r(const char *name, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result)
{
	*result = NULL;
	if( strcmp(name, "root") == 0 ) {
		int ret = fill_pwd(&gPwd_RootInfo, pwd, buf, buflen);
		if(ret)	return ret;
		*result = pwd;
	}
	return 0;
}

int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, size_t buflen, struct passwd **result)
{
	*result = NULL;
	if( uid == 0 ) {
		int ret = fill_pwd(&gPwd_RootInfo, pwd, buf, buflen);
		if(ret)	return ret;
		*result = pwd;
	}
	return 0;
}

void endpwent(void)
{
}

struct passwd *getpwent(void)
{
	return NULL;
}

void setpwent(void)
{

}

