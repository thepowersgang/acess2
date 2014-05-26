/*
 * Acess2
 * - C Runtime 0 Common Code
 */

typedef	void	(*exithandler_t)(void);
typedef	void	(*constructor_t)(void);

constructor_t	_crtbegin_ctors[0] __attribute__((section(".ctors")));

exithandler_t	_crt0_exit_handler;
//extern constructor_t	_crtbegin_ctors[];
extern void	_init(void);
extern void	_fini(void);
extern void	_exit(int status) __attribute__((noreturn));
extern int	main(int argc, char *argv[], char **envp);

void _start(int argc, char *argv[], char **envp) __attribute__ ((alias("start")));

void start(int argc, char *argv[], char **envp)
{
	 int	i;
	 int	rv;
	
	for( i = 0; _crtbegin_ctors[i]; i ++ )
		_crtbegin_ctors[i]();
	
	_init();

	rv = main(argc, argv, envp);
	
	if( _crt0_exit_handler )
		_crt0_exit_handler();
	_fini();
	_exit(rv);
}
