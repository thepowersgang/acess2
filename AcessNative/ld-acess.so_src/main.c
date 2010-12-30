/*
 */
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

// === CODE ===
int main(int argc, char *argv[], char **envp)
{
	 int	i;
	 int	appArgc;
	char	**appArgv;
	char	*appPath;
	 int	(*appMain)(int, char *[], char **);
	void	*base;
	
	for( i = 1; i < argc; i ++ )
	{
		if( argv[i][0] != '-' )	break;
	}

	if( i >= argc ) {
		fprintf(stderr, "Usage: ld-acess <executable> [arguments ...]\n");
		return 1;
	}
	
	appPath = argv[i];

	appArgc = argc - i;
	appArgv = &argv[i];

	printf("Exectutable Path: '%s'\n", appPath);
	printf("Executable argc = %i\n", appArgc);

	base = Binary_Load(appPath, (uintptr_t*)&appMain);
	printf("base = %p\n", base);
	if( !base )	return 127;
	
	return appMain(appArgc, appArgv, envp);
}

void Warning(const char *Format, ...)
{
	va_list	args;
	printf("Warning: ");
	va_start(args, Format);
	vprintf(Format, args);
	va_end(args);
	printf("\n");
}

void Notice(const char *Format, ...)
{
	va_list	args;
	printf("Notice: ");
	va_start(args, Format);
	vprintf(Format, args);
	va_end(args);
	printf("\n");
}

