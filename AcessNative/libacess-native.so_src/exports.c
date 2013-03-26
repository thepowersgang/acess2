
#include "../ld-acess_src/exports.c"

void	*_crt0_exit_handler;

int *libc_geterrno(void)
{
	return &acess__errno;
}


