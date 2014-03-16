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
#include <stdlib.h>
#include <time.h>

extern int	VFS_Init(void);
extern int	IPStack_Install(char **Args);

// === CODE ===
void PrintUsage(const char *ProgramName)
{
	fprintf(stderr, "Usage: %s <commands...>\n", ProgramName);
	fprintf(stderr, "\n");
	fprintf(stderr,
		"Options:\n"
		"-dev <mac>:<type>:<arg>\n"
		"-ip <dev>,<addr>/<mask>\n"
		"-route <net>/<mask>,<nexthop>\n"
		"\n"
		"Commands:\n"
		"netcat <addr> <port>\n"
		"\n"
		"Device Types:\n"
		"tun - Linux TUN/TAP device (takes an optional name)\n"
		"unix - Unix named pipe (datagram)\n"
		);
}

int main(int argc, char *argv[])
{
	 int	rv;
	
	if( argc <= 1 ) {
		PrintUsage(argv[0]);
		return 1;
	}

	srand(time(NULL));	

	// Startup
	VFS_Init();
	{
		char	*ipstack_args[] = {NULL};
		IPStack_Install( ipstack_args );
	}
	
	for( int i = 1; i < argc; i ++ )
	{
		char *arg = argv[i];;
		if( arg[0] != '-' )
		{
			if( strcmp(arg, "netcat") == 0 )
			{
				if( argc-i != 3 ) {
					Log_Error("NetTest", "'netcat' <addr> <port>");
					PrintUsage(argv[0]);
					return -1;
				}

				NetTest_Suite_Netcat(argv[i+1], strtol(argv[i+2], NULL, 0));
				i += 2;
			}
			else if( strcmp(arg, "cmdline") == 0 )
			{
				NetTest_Suite_Cmdline();
			}
			else
			{
				Log_Error("NetTest", "Unknown suite name '%s'", arg);
				PrintUsage(argv[0]);
			}
		}
		else
		{
			if( strcmp(arg, "--help") == 0 )
			{
				PrintUsage(0);
				return 0;
			}
			else if( strcmp(arg, "-dev") == 0 )
			{
				if( i+1 == argc ) { PrintUsage(argv[0]); return 1; }
				rv = NativeNic_AddDev(argv[i+1]);
				if( rv ) {
					Log_Error("NetTest", "Failed to add device %s", argv[i+1]);
					return -1;
				}
				i ++;
			}
			else if( strcmp(arg, "-ip") == 0 )
			{
				if( i+1 == argc ) { PrintUsage(argv[0]); return 1; }
				// Parse argument and poke ipstack
				if( NetTest_AddAddress(argv[i+1]) ) {
					return -1;
				}
				i ++;
			}
			else
			{
				Log_Error("NetTest", "Unknown argument '%s'", arg);
				PrintUsage(argv[0]);
				return -1;
			}
		}
	}

	// Teardown
	Log_Log("NetTest", "Shutting down");
	fflush(stdout);
	return 0;
}

size_t NetTest_WriteStdout(const void *Data, size_t Size)
{
	return fwrite(Data, 1, Size, stdout);
}
