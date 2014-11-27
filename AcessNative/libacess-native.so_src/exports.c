#include "common.h"

#define acess__SysSpawn	_disabled_acess__SysSpawn
#include "../ld-acess_src/exports.c"

void	*_crt0_exit_handler;

int *libc_geterrno(void)
{
	return &acess__errno;
}

void _ZN4_sys5debugEPKcz(const char *fmt, ...)	__attribute__((alias("acess__SysDebug")));
void _ZN4_sys7hexdumpEPKcPKvj(const char *tag, const void *ptr, size_t size) __attribute__((alias("acess__SysDebugHex")));

#undef acess__SysSpawn

int acess__SysSpawn(const char *binary, const char **argv, const char **envp, int nfd, int fds[], struct s_sys_spawninfo *info)
{
	 int	argc = 0;
	while( argv[argc++] )
		;

	Debug("_SysSpawn('%s', %p (%i), %p, %i, %p, %p)",
		binary, argv, argc, envp, nfd, fds, info);

	char realpath[256];
	realpath[255] = 0;

	if( _Syscall(SYS_AN_GETPATH, "<d >s", sizeof(realpath)-1, realpath, binary) <= 0 ) {
		Warning("No translation for path '%s'", binary);
		acess__errno = -11;
		return -1;
	}

	Warning("TODO: Spawn '%s' = '%s'", binary, realpath);

	 int	emulated_tid;
	int newID = _Syscall(SYS_AN_SPAWN, "<d >d >d",
		sizeof(emulated_tid), &emulated_tid,
		nfd*sizeof(int), fds,
		(info ? sizeof(*info) : 0), info
		);

	if( newID <= 0 ) {
		return -1;
	}

	if( acessnative_spawn(realpath, newID, argv, envp) ) {
	}

	return emulated_tid;
}

void ldacess_DumpLoadedLibraries(void)
{
	Debug("ldacess_DumpLoadedLibraries");
}

