/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * Kernel Main
 */
#include <stdio.h>
#include <stdlib.h>

// === IMPORTS ===
extern int	UI_Initialise(int Width, int Height);
extern int	VFS_Init(void);
extern int	Video_Install(char **Arguments);
extern int	NativeKeyboard_Install(char **Arguments);
extern int	VT_Install(char **Arguments);
extern int	VFS_Mount(const char *Device, const char *MountPoint, const char *Filesystem, const char *Options);
extern int	SyscallServer(void);

// === GLOBALS ===
const char	*gsAcessDir = "../Usermode/Output/i386";

// === CODE ===
int main(int argc, char *argv[])
{
	// Parse command line settings
	
	// Start UI subsystem
	UI_Initialise(800, 480);
	
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
	
	return 0;
}

void AcessNative_Exit(void)
{
	// TODO: Close client applications too
	exit(0);
}
