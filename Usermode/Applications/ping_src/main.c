/*
 * Acess2 IFCONFIG command
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <acess/sys.h>
#include <net.h>

// === CONSTANTS ===
#define IPSTACK_ROOT	"/Devices/ip"

// === PROTOTYPES ===
void	PrintUsage(char *ProgName);
void	PrintHelp(char *ProgName);

// === GLOBALS ===
 int	giNumberOfPings = 1;

// === CODE ===
/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	char	*ipStr = NULL;
	char	*iface = NULL;
	 int	i, j;
	uint8_t	addr[16];
	 int	type;
	 
	 int	fd, call, ping;
	
	for(i = 1; i < argc; i++)
	{
		if(argv[i][0] != '-')
		{
			if(!ipStr)
				ipStr = argv[i];
			else if(!iface)
				iface = argv[i];
			else {
				PrintUsage(argv[0]);
				return 1;
			}
		}
		
		if(argv[i][1] == '-')
		{
			char	*arg = &argv[i][2];
			if(strcmp(arg, "help") == 0) {
				PrintHelp(argv[1]);
				return 0;
			}
		}
		
		for( j = 1; argv[i][j]; j++ )
		{
			switch(argv[i][j])
			{
			case '?':
			case 'h':
				PrintHelp(argv[1]);
				return 0;
			}
		}
	}
	
	if(!ipStr) {
		PrintUsage(argv[0]);
		return 1;
	}
	
	// Read Address
	type = Net_ParseAddress(ipStr, addr);
	if( type == 0 ) {
		fprintf(stderr, "Invalid IP Address\n");
		return 1;
	}
	
	if( iface )
	{
		char	*_iface = malloc( sizeof("/Devices/ip/") + strlen(iface) + 1 );
		strcpy(_iface, "/Devices/ip/");
		strcat(_iface, iface);
		free(iface);	// TODO: Handle when this is not heap
		iface = _iface;
		printf("iface = '%s'\n", iface);
		fd = open(iface, OPENFLAG_EXEC);
		if(fd == -1) {
			fprintf(stderr, "ERROR: Unable to open interface '%s'\n", iface);
			return 1;
		}
		
	}
	else
	{
		fd = Net_OpenSocket(type, addr, NULL);
		if( fd == -1 ) {
			fprintf(stderr, "Unable to find a route to '%s'\n",
				Net_PrintAddress(type, addr)
				);
			return -1;
		}
	}
	
	call = ioctl(fd, 3, "ping");
	if(call == 0) {
		fprintf(stderr, "ERROR: '%s' does not have a 'ping' call\n", iface);
		return 1;
	}
	
	for( i = 0; i < giNumberOfPings; i ++ )
	{
		ping = ioctl(fd, call, addr);
		printf("ping = %i\n", ping);
	}
		
	close(fd);
	
	return 0;
}

void PrintUsage(char *ProgName)
{
	fprintf(stderr, "Usage: %s <address> [<interface>]\n", ProgName);
}

void PrintHelp(char *ProgName)
{
	PrintUsage(ProgName);
	fprintf(stderr, " -h\tPrint this message\n");
}

