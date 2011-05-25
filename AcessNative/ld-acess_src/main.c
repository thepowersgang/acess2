/*
 */
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// === CODE ===
int main(int argc, char *argv[], char **envp)
{
	 int	i;
	 int	appArgc;
	char	**appArgv;
	char	*appPath = NULL;
	 int	(*appMain)(int, char *[], char **);
	void	*base;
	
	 int	syscall_handle = -1;
	
	for( i = 1; i < argc; i ++ )
	{
		if(strcmp(argv[i], "--key") == 0) {
			syscall_handle = atoi(argv[++i]);
			continue ;
		}
		
		if(strcmp(argv[i], "--binary") == 0) {
			appPath = argv[++i];
			continue ;
		}
		
		if(strcmp(argv[i], "--open") == 0) {
			if( acess_open(argv[++i], 6) == -1 ) {	// Read/Write
				fprintf(stderr, "Unable to open '%s'\n", argv[i]);
				exit(1);
			}
			continue ;
		}
		
		if( argv[i][0] != '-' )	break;
	}

	if( i >= argc ) {
		fprintf(stderr, "Usage: ld-acess <executable> [arguments ...]\n");
		return 1;
	}
	
	if( !appPath )
		appPath = argv[i];

	appArgc = argc - i;
	appArgv = &argv[i];

	printf("Exectutable Path: '%s'\n", appPath);
	printf("Executable argc = %i\n", appArgc);

	base = Binary_Load(appPath, (uintptr_t*)&appMain);
	printf("base = %p\n", base);
	if( !base )	return 127;
	
	printf("==============================\n");
	for(i = 0; i < appArgc; i ++)
		printf("\"%s\" ", appArgv[i]);
	printf("\n");
	__asm__ __volatile__ (
		"push %0;\n\t"
		"push %1;\n\t"
		"push %2;\n\t"
		"jmp *%3;\n\t"
		: : "r" (envp), "r" (appArgv), "r" (appArgc), "r" (appMain) );
	return -1;
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

