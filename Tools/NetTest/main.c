/*
 * Acess2 Networking Test Suite (NetTest)
 * - By John Hodge (thePowersGang)
 *
 * main.c
 * - Program Core
 */
#include <stdio.h>
#include <acess_logging.h>
#include <nettest.h>
#include <string.h>

extern int	IPStack_Install(char **Args);

// === CODE ===
void PrintUsage(const char *ProgramName)
{
	fprintf(stderr, "Usage: %s <commands...>\n", ProgramName);
	fprintf(stderr, "\n");
	fprintf(stderr,
		"-dev <tapdev>:<mac>\n"
		"-ip <dev>,<addr>,<mask>\n"
		"-route <net>,<mask>,<nexthop>\n"
		);
}

int main(int argc, char *argv[])
{
	if( argc <= 1 ) {
		PrintUsage(argv[0]);
		return 1;
	}
	
	// Startup
	{
		char	*ipstack_args[] = {NULL};
		IPStack_Install( ipstack_args );
	}
	
	for( int i = 0; i < argc; i ++ )
	{
		if( argv[i][0] != '-' ) {
		}
		else if( strcmp(argv[i], "-dev") == 0 )
		{
			if( ++i == argc ) { PrintUsage(argv[0]); return 1; }
			NativeNic_AddDev(argv[i]);
		}
	}

	// Run suite
	

	// Teardown

	return 0;
}
