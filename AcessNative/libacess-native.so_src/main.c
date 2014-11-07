/*
 */
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include "common.h"
#include <stdint.h>
#include "../ld-acess_src/exports.h"

extern int	gbSyscallDebugEnabled;

#ifdef __WINDOWS__
int DllMain(void)
{
	fprintf(stderr, "TODO: Windows libacessnative setup\n");
	return 0;
}

#endif

#ifdef __linux__
int __attribute__ ((constructor(102))) libacessnative_init(int argc, char *argv[], char **envp);

const char *getenv_p(char **envp, const char *name)
{
	size_t	namelen = strlen(name);
	for(; *envp; envp ++)
	{
		if( strncmp(*envp, name, namelen) != 0 )
			continue ;
		if( (*envp)[namelen] != '=' )
			continue ;
		return (*envp)+namelen+1;
	}
	return 0;
}

int libacessnative_init(int argc, char *argv[], char **envp)
{
	Request_Preinit();
	
	//gbSyscallDebugEnabled = 1;
	
	const char *preopens = getenv_p(envp, ENV_VAR_PREOPENS);
	printf("preopens = %s\n", preopens);
	if( preopens )
	{
		 int	exp_fd = 0;
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
				fprintf(stderr, "Unable to preopen '%s' errno=%i\n", path, acess__errno);
				exit(1);
			}
			if( fd != exp_fd ) {
				//  Oh... this is bad
				fprintf(stderr, "Pre-opening '%s' resulted in an incorrect FD (expected %i, got %i)",
					path, exp_fd, fd);
				exit(1);
			}
			exp_fd += 1;
			
			if( !splitter )
				break;
			preopens = splitter + 1;
		}
	}

//	if( !getenv(ENV_VAR_KEY)
	
	return 0;
}
#endif

int acessnative_spawn(const char *Binary, int SyscallID, const char * const * argv, const char * const * envp)
{
	 int	envc = 0;
	while( envp && envp[envc++] )
		envc ++;

	// Set environment variables for libacess-native
	// > ACESSNATIVE_KEY=`newID`
	size_t keystr_len = snprintf(NULL, 0, "%s=%i", ENV_VAR_KEY, SyscallID);
	char keystr[keystr_len+1];
	snprintf(keystr, keystr_len+1, "%s=%i", ENV_VAR_KEY, SyscallID);
	bool	bKeyHit = false;
	
	const char *newenv[envc+2+1];
	 int	i = 0;
	for( ; envp && envp[i]; i ++ )
	{
		const char	*ev = envp[i];
		if( strncmp(ev, ENV_VAR_KEY"=", sizeof(ENV_VAR_KEY"=")) == 0 ) {
			ev = keystr;
			bKeyHit = true;
		}
		newenv[i] = ev;
	}
	if( !bKeyHit )
		newenv[i++] = keystr;
	newenv[i++] = getenv("LD_LIBRARY_PATH") - (sizeof("LD_LIBRARY_PATH=")-1);	// VERY hacky
	newenv[i] = NULL;
	
	// TODO: Detect native_spawn failing
	return native_spawn(Binary, argv, newenv);
}

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

