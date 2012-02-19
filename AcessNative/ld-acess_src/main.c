/*
 */
#include "common.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// === IMPORTS ===
extern int	gSocket;
extern int	giSyscall_ClientID;
extern void	acess__exit(int Status);
extern void	Request_Preinit(void);

// === PROTOTYPES ===
void	CallUser(void *Entry, int argc, char *argv[], char **envp) __attribute__((noreturn));

// === CODE ===
int main(int argc, char *argv[], char **envp)
{
	 int	i;
	 int	appArgc;
	char	**appArgv;
	char	*appPath = NULL;
	 int	(*appMain)(int, char *[], char **);
	void	*base;
	 int	rv;
	
	Request_Preinit();

//	 int	syscall_handle = -1;
	
	for( i = 1; i < argc; i ++ )
	{
		if(strcmp(argv[i], "--key") == 0) {
			giSyscall_ClientID = atoi(argv[++i]);
			continue ;
		}

		if(strcmp(argv[i], "--socket") == 0) {
			gSocket = atoi(argv[++i]);
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
		fprintf(stderr,
			"Usage: ld-acess <executable> [arguments ...]\n"
			"\n"
			"--key\t(internal) used to pass the system call handle when run with execve\n"
			"--binary\tLoad a local binary directly\n"
			"--open\tOpen a file before executing\n"
			);
		return 1;
	}
	
	if( !appPath )
		appPath = argv[i];

	appArgc = argc - i;
	appArgv = &argv[i];

//	printf("Exectutable Path: '%s'\n", appPath);
//	printf("Executable argc = %i\n", appArgc);

	base = Binary_Load(appPath, (uintptr_t*)&appMain);
	printf("[DEBUG %i] base = %p\n", giSyscall_ClientID, base);
	if( !base )	return 127;
	
	printf("==============================\n");
	printf("[DEBUG %i] %i ", giSyscall_ClientID, appArgc);
	for(i = 0; i < appArgc; i ++)
		printf("\"%s\" ", appArgv[i]);
	printf("\n");
	printf("[DEBUG %i] appMain = %p\n", giSyscall_ClientID, appMain);
//	CallUser(appMain, appArgc, appArgv, envp);
	rv = appMain(appArgc, appArgv, envp);
	acess__exit(rv);
	return rv;
}

void CallUser(void *Entry, int argc, char *argv[], char **envp)
{
	#if 0
	__asm__ __volatile__ (
		"push %3;\n\t"
		"push %2;\n\t"
		"push %1;\n\t"
		"call *%0"
		: : "r" (Entry), "r" (argc), "r" (argv), "r" (envp)
		);
	#else
	#endif
	for(;;);
}

void Warning(const char *Format, ...)
{
	va_list	args;
	printf("[WARN   %i] ", giSyscall_ClientID);
	va_start(args, Format);
	vprintf(Format, args);
	va_end(args);
	printf("\n");
}

void Notice(const char *Format, ...)
{
	va_list	args;
	printf("[NOTICE %i] ", giSyscall_ClientID);
	va_start(args, Format);
	vprintf(Format, args);
	va_end(args);
	printf("\n");
}

void Debug(const char *Format, ...)
{
	va_list	args;
	printf("[DEBUG  %i] ", giSyscall_ClientID);
	va_start(args, Format);
	vprintf(Format, args);
	va_end(args);
	printf("\n");
}

