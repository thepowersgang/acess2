/*
 * Acess2 System Init Task
 */
#include <acess/sys.h>

// === CONSTANTS ===
#define NULL	((void*)0)
#define	DEFAULT_TERMINAL	"/Devices/VTerm/0"
#define DEFAULT_SHELL	"/Acess/CLIShell"

// === CODE ===
/**
 * \fn int main(int argc, char *argv[])
 */
int main(int argc, char *argv[])
{
	 int	tid;
	open(DEFAULT_TERMINAL, OPENFLAG_READ);	// Stdin
	open(DEFAULT_TERMINAL, OPENFLAG_WRITE);	// Stdout
	open(DEFAULT_TERMINAL, OPENFLAG_WRITE);	// Stderr
	
	write(1, 13, "Hello, World!");
	
	tid = clone(CLONE_VM, 0);
	__asm__ __volatile__("xchg %%bx, %%bx"::"a"(tid));
	if(tid == 0)
	{
		execve(DEFAULT_SHELL, NULL, NULL);
		for(;;) __asm__ __volatile__("hlt");
	}
	
	for(;;)	sleep();
	
	return 42;
}
