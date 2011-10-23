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

// === IMPORTS ===
extern int	UI_Initialise(int Width, int Height);
extern void	UI_MainLoop(void);
extern int	VFS_Init(void);
extern int	Video_Install(char **Arguments);
extern int	NativeKeyboard_Install(char **Arguments);
extern int	NativeFS_Install(char **Arguments);
extern void	Debug_SetKTerminal(char *Path);
extern int	VT_Install(char **Arguments);
extern int	VFS_Mount(const char *Device, const char *MountPoint, const char *Filesystem, const char *Options);
extern int	SyscallServer(void);
extern const char	gsKernelVersion[];
extern const char	gsGitHash[];
extern int	giBuildNumber;

// === GLOBALS ===
const char	*gsAcessDir = "../Usermode/Output/i386";

// === CODE ===
int main(int argc, char *argv[])
{
	// Parse command line settings
	printf("Acess2 Native v%s\n", gsKernelVersion);
	printf(" Build %i, Git Hash %s\n", giBuildNumber, gsGitHash);

	// Start UI subsystem
	UI_Initialise(800, 480);
	
	// - Ignore SIGUSR1 (used to wake threads)
	signal(SIGUSR1, SIG_IGN);
		
	// Initialise VFS
	VFS_Init();
	// - Start IO Drivers
	Video_Install(NULL);
	NativeKeyboard_Install(NULL);
	NativeFS_Install(NULL);
	// - Start VTerm
	{
		char	*args[] = {
			"Video=NativeVideo",
			"Input=NativeKeyboard",
			NULL
			};
		VT_Install(args);
	}
	
	VFS_Mount(gsAcessDir, "/Acess", "nativefs", "");

	Debug_SetKTerminal("/Devices/VTerm/8");
	
	// Start syscall server
	// - Blocks
	SyscallServer();
	
	UI_MainLoop();

	return 0;
}

void AcessNative_Exit(void)
{
	// TODO: Close client applications too
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

