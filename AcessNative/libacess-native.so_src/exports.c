
#define acess__SysSpawn	_disabled_acess__SysSpawn
#include "../ld-acess_src/exports.c"

void	*_crt0_exit_handler;

int *libc_geterrno(void)
{
	return &acess__errno;
}

#undef acess__SysSpawn

int acess__SysSpawn(const char *binary, const char **argv, const char **envp, int nfd, int fds[], struct s_sys_spawninfo *info)
{
	 int	argc = 0;
	while( argv[argc++] );

	Debug("_SysSpawn('%s', %p (%i), %p, %i, %p, %p)",
		binary, argv, argc, envp, nfd, fds, info);

	 int	kernel_tid;
	 int	newID;
	newID = _Syscall(SYS_AN_SPAWN, "<d >d >d", sizeof(int), &kernel_tid,
		nfd*sizeof(int), fds,
		info ? sizeof(*info) : 0, info);
	

	// TODO: Translate internal path to actual path	

	// TODO: set environment variables for libacess-native
	// > ACESSNATIVE_KEY=`newID`
	native_spawn(binary, argv, envp);

	return 0;
}

