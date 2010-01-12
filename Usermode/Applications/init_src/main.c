/*
 * Acess2 System Init Task
 */
#include <acess/sys.h>

// === CONSTANTS ===
#define NULL	((void*)0)
#define NUM_TERMS	4
#define	DEFAULT_TERMINAL	"/Devices/VTerm/0"
#define DEFAULT_SHELL	"/Acess/SBin/login"

// === CODE ===
/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	 int	tid;
	 int	i;
	char	termpath[sizeof(DEFAULT_TERMINAL)] = DEFAULT_TERMINAL;
	char	*child_argv[2] = {DEFAULT_SHELL, 0};
	
	for( i = 0; i < NUM_TERMS; i++ )
	{		
		tid = clone(CLONE_VM, 0);
		if(tid == 0)
		{
			termpath[sizeof(DEFAULT_TERMINAL)-2] = '0' + i;
			
			//__asm__ __volatile__ ("int $0xAC" :: "a" (256), "b" ("%s"), "c" (termpath));
			
			open(termpath, OPENFLAG_READ);	// Stdin
			open(termpath, OPENFLAG_WRITE);	// Stdout
			open(termpath, OPENFLAG_WRITE);	// Stderr
			execve(DEFAULT_SHELL, child_argv, NULL);
			for(;;)	sleep();
		}
	}
	
	// TODO: Implement message watching
	for(;;)	sleep();
	
	return 42;
}
