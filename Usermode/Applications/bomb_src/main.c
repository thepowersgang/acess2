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
		
		switch( argv[i][0] )
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
		for(;;)	clone(0, malloc(512-16)+512-16);
	}

	return 0;
}
