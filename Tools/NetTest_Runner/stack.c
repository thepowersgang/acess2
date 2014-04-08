/*
 */
#define _POSIX_C_SOURCE 200809L
#include "stack.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdarg.h>

#include <fcntl.h>
#include <spawn.h>

#define MAX_ARGS	16

extern char **environ;

// === GLOBALS ===
pid_t	giStack_PID;
 int	giStack_InFD = -1;
 int	giNumStackArgs = 1;
char	*gasStackArgs[MAX_ARGS] = {"nettest",NULL};

// === CODE ===
void Stack_AddDevice(const char *Ident, const void *MacAddr)
{
	Stack_AddArg("-dev");
	Stack_AddArg("%02x%02x%02x%02x%02x%02x:unix:%s",
		((uint8_t*)MacAddr)[0],
		((uint8_t*)MacAddr)[1],
		((uint8_t*)MacAddr)[2],
		((uint8_t*)MacAddr)[3],
		((uint8_t*)MacAddr)[4],
		((uint8_t*)MacAddr)[5],
		Ident);
}

void Stack_AddInterface(const char *Name, int AddrType, const void *Addr, int MaskBits)
{
	Stack_AddArg("-ip");
	switch(AddrType)
	{
	case 4:
		Stack_AddArg("%s,%i.%i.%i.%i/%i", Name,
			((uint8_t*)Addr)[0],
			((uint8_t*)Addr)[1],
			((uint8_t*)Addr)[2],
			((uint8_t*)Addr)[3],
			MaskBits
			);
		break;
	default:
		assert(AddrType == 4);
		exit(1);
	}
}

void Stack_AddRoute(int Type, const void *Network, int MaskBits, const void *NextHop)
{
}

void Stack_AddArg(const char *Fmt, ...)
{
	va_list	args;
	va_start(args, Fmt);
	size_t len = vsnprintf(NULL, 0, Fmt, args);
	va_end(args);
	char *arg = malloc(len+1);
	va_start(args, Fmt);
	vsnprintf(arg, len+1, Fmt, args);
	va_end(args);
	gasStackArgs[giNumStackArgs++] = arg;
}

void sigchld_handler(int signum)
{
	 int status;
	wait(&status);
	fprintf(stderr, "FAILURE: Child exited (%i)\n", status);
}

int Stack_Start(const char *Subcommand)
{
	Stack_AddArg(Subcommand);
	
	for( int i = 3; i < 16; i ++ )
		fcntl(i, F_SETFD, FD_CLOEXEC);

	//signal(SIGCHLD, sigchld_handler);

	 int	fds_in[2];
	pipe(fds_in);
	giStack_InFD = fds_in[1];
	fcntl(giStack_InFD, F_SETFD, FD_CLOEXEC);

	FILE	*fp;
	fp = fopen("stdout.txt", "a"); fprintf(fp, "--- Startup\n"); fclose(fp);
	fp = fopen("stderr.txt", "a"); fprintf(fp, "--- Startup\n"); fclose(fp);

	posix_spawn_file_actions_t	fa;
	posix_spawn_file_actions_init(&fa);
	posix_spawn_file_actions_adddup2(&fa, fds_in[0], 0);
	posix_spawn_file_actions_addopen(&fa, 1, "stdout.txt", O_CREAT|O_APPEND|O_WRONLY, 0644);
	posix_spawn_file_actions_addopen(&fa, 2, "stderr.txt", O_CREAT|O_APPEND|O_WRONLY, 0644);

	int rv = posix_spawn(&giStack_PID, "./nettest", &fa, NULL, gasStackArgs, environ);
	if(rv) {
		giStack_PID = 0;
		fprintf(stderr, "posix_spawn failed: %s", strerror(rv));
		return 1;
	}
	
	posix_spawn_file_actions_destroy(&fa);

	return 0;
}
void Stack_Kill(void)
{
	if( giStack_PID )
	{
		kill(giStack_PID, SIGTERM);
		giStack_PID = 0;
	}
}

int Stack_SendCommand(const char *Fmt, ...)
{
	va_list	args;
	va_start(args, Fmt);
	size_t len = vsnprintf(NULL, 0, Fmt, args);
	va_end(args);
	char command[len+1];
	va_start(args, Fmt);
	vsnprintf(command, len+1, Fmt, args);
	va_end(args);
	
	write(giStack_InFD, command, len);
	write(giStack_InFD, "\n", 1);
	return 0;
}

