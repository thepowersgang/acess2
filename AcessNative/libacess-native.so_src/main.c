/*
 */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

extern int	giSyscall_ClientID;
extern void	Request_Preinit(void);
extern int	acess__SysOpen(const char *Path, unsigned int flags);

#ifdef __WINDOWS__
int DllMain(void)
{
	fprintf(stderr, "TODO: Windows libacessnative setup\n");
	return 0;
}

#endif

#ifdef __linux__
int __attribute__ ((constructor)) libacessnative_init(void);

int libacessnative_init(void)
{
	Request_Preinit();

	const char *preopens = getenv("AN_PREOPEN");
	if( preopens )
	{
		while( *preopens )
		{
			const char *splitter = strchr(preopens, ':');
			size_t	len;
			if( !splitter ) {
				len = strlen(preopens);
			}
			else {
				len = splitter - preopens;
			}
			char path[len+1];
			memcpy(path, preopens, len);
			path[len] = 0;
			int fd = acess__SysOpen(path, 6);	// WRITE,READ,no EXEC
			if( fd == -1 ) {
				fprintf(stderr, "Unable to preopen '%s'\n", path);
			}
			
			if( !splitter )
				break;
			preopens = splitter + 1;
		}
	}

//	if( !getenv("ACESSNATIVE_ID")
	
	return 0;
}
#endif


void Debug(const char *format, ...)
{
	va_list	args;
	printf("Debug: ");
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	printf("\n");
}

void Warning(const char *format, ...)
{
	va_list	args;
	printf("Warning: ");
	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);
	printf("\n");
}

void __libc_csu_fini()
{
}

void __libc_csu_init()
{
}

