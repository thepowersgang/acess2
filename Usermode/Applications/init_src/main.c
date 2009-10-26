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
	// int	i;
	char	termpath[sizeof(DEFAULT_TERMINAL)+1] = DEFAULT_TERMINAL;
	
	//for( i = 0; i < NUM_TERMS; i++ )
	//{
		//termpath[ sizeof(DEFAULT_TERMINAL)-1 ] = '0' + i;
		open(termpath, OPENFLAG_READ);	// Stdin
		open(termpath, OPENFLAG_WRITE);	// Stdout
		open(termpath, OPENFLAG_WRITE);	// Stderr
		
		tid = clone(CLONE_VM, 0);
		if(tid == 0)
		{
			execve(DEFAULT_SHELL, NULL, NULL);
			for(;;) __asm__ __volatile__("hlt");
		}
	//}
	
	for(;;)	sleep();
	
	return 42;
}
