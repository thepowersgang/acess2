/*
 * Acess2 Stress Tester
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>

 //int	giMallocSize = 1024;
 int	gbOverrideCOW = 0;
 int	gbForkBomb = 0;

/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	 int	i;
	
	for( i = 1; i < argc; i ++ )
	{
		if( argv[i][0] != '-' ) {
			//PrintUsage(argv[0]);
			return 1;
		}
		
		switch( argv[i][1] )
		{
		case 'f':
			gbForkBomb = 1;
			break;
		case 'c':
			gbOverrideCOW = 1;
			break;
		//case 'm':
		//	giMallocSize = atoi(argv[++i]);
		//	break;
		case 'h':
			//PrintHelp(argv[0]);
			return 0;
		}
	}

	if( gbForkBomb )
	{
		for(;;)	clone(CLONE_VM, 0);
	}
	else {
		for(;;)
		{
			const int stackSize = 512-16; 
			const int stackOffset = 65; 
			char *stack = calloc(1, stackSize);
			 int	tid;
			if( !stack ) {
				printf("Outta heap space!\n");
				return 0;
			}
			tid = clone(0, stack+stackSize-stackOffset);
			//_SysDebug("tid = %i", tid);
			if( tid == 0 )
			{
				// Sleep forever (TODO: Fix up the stack so it can nuke)
				for(;;) _SysWaitEvent(THREAD_EVENT_SIGNAL);
			}
			if( tid < 0 ) {
				printf("Clone failed\n");
				return 0;
			}
		}
	}

	printf("RETURN!?\n");
	return 0;
}
