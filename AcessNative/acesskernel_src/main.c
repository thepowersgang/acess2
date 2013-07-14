/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * Kernel Main
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <stdint.h>
#ifdef __LINUX__
#include <sys/prctl.h>
#endif
#include <unistd.h>
#include <string.h>
#include "../../KernelLand/Kernel/include/logdebug.h"

#define VALGRIND_CLIENT	0

// === IMPORTS ===
extern int	UI_Initialise(int Width, int Height);
extern void	UI_MainLoop(void);
extern int	VFS_Init(void);
extern int	PTY_Install(char **Arguments);
extern int	Video_Install(char **Arguments);
extern int	NativeKeyboard_Install(char **Arguments);
extern int	NativeFS_Install(char **Arguments);
extern void	Debug_SetKTerminal(char *Path);
extern int	VT_Install(char **Arguments);
extern int	Mouse_Install(char **Arguments);
extern int	IPCPipe_Install(char **Arguments);
extern int	VFS_Mount(const char *Device, const char *MountPoint, const char *Filesystem, const char *Options);
extern int	VFS_MkDir(const char *Path);
extern int	SyscallServer(void);
extern int	Server_Shutdown(void);
extern const char	gsKernelVersion[];
extern const char	gsGitHash[];
extern int	giBuildNumber;

// === GLOBALS ===
const char	*gsAcessDir = "../Usermode/Output/x86_64";

// === CODE ===
#ifndef __WIN32__
#define P_NOWAIT	0
int spawnv(int flags, const char *execuable, char * const argv[])
{
	int pid = fork();
	if( pid != 0 )	return pid;

	execv(execuable, argv);
	perror("spawnv - execve");
	for(;;);
}
#endif

int main(int argc, char *argv[])
{
	char	**rootapp = NULL;
	 int	rootapp_argc, i;
	// Parse command line settings
	for( i = 1; i < argc; i ++ )
	{
		if( strcmp(argv[i], "--distroot") == 0 ) {
			gsAcessDir = argv[++i];
		}
		else if( strcmp(argv[i], "--rootapp") == 0 ) {
			rootapp = &argv[++i];
			rootapp_argc = argc - i;
			break;
		}
		else {
			fprintf(stderr, "Unknown command line option '%s'\n", argv[i]);
			return -1;
		}
	}	

	// Kernel build information
	printf("Acess2 Native v%s\n", gsKernelVersion);
	printf(" Build %i, Git Hash %s\n", giBuildNumber, gsGitHash);

	// Start UI subsystem
	UI_Initialise(800, 480);
	
	// - Ignore SIGUSR1 (used to wake threads)
	#ifdef SIGUSR1
	signal(SIGUSR1, SIG_IGN);
	#endif
		
	// Initialise VFS
	VFS_Init();
	// - Start IO Drivers
	if( Video_Install(NULL) ) {
		Log_Error("Init", "Unable to load NativeVideo");
	}
	if( NativeKeyboard_Install(NULL) ) {
		Log_Error("Init", "Unable to load NativeKeyboard");
	}
	NativeFS_Install(NULL);
	Mouse_Install(NULL);
	IPCPipe_Install(NULL);
	PTY_Install(NULL);
	// - Start VTerm
	{
		char	*args[] = {
			"Video=NativeVideo",
			"Input=NativeKeyboard",
			NULL
			};
		VT_Install(args);
	}

	VFS_MkDir("/Acess");	
	VFS_Mount(gsAcessDir, "/Acess", "nativefs", "");

	Debug_SetKTerminal("/Devices/pts/vt7c");
	
	// Start syscall server
	SyscallServer();
	
	// Spawn root application
	if( rootapp )
	{
		 int	pid;
		 int	argcount = 0;
		const char	*args[7+rootapp_argc+1+1];

		#if VALGRIND_CLIENT
		args[argcount++] = "valgrind";
		#endif
		args[argcount++] = "./ld-acess";
		args[argcount++] = "--open";	args[argcount++] = "/Devices/VTerm/0";
		args[argcount++] = "--open";	args[argcount++] = "/Devices/VTerm/0";
		args[argcount++] = "--open";	args[argcount++] = "/Devices/VTerm/0";
		for( i = 0; i < rootapp_argc; i ++ )
			args[argcount+i] = rootapp[i];
		args[argcount+rootapp_argc] = NULL;
		pid = spawnv(P_NOWAIT, "./ld-acess", args);
		if(pid < 0) {
			perror("Starting root application [fork(2)]");
			return 1;
		}
		printf("Root application running as PID %i\n", pid);
	}

	UI_MainLoop();

	return 0;
}

void AcessNative_Exit(void)
{
	// TODO: Close client applications too
	Server_Shutdown();
	exit(0);
}

uint64_t DivMod64U(uint64_t Num, uint64_t Den, uint64_t *Rem)
{
	if(Rem)	*Rem = Num % Den;
	return Num / Den;
}

int Module_EnsureLoaded(const char *Name)
{
	return 0;
}

