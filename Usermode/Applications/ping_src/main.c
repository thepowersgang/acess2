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
 int	GetAddress( char *Address, uint8_t *Addr );

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
	
	if( !iface )
	{
		iface = Net_GetInterface(type, addr);
		if( !iface ) {
			fprintf(stderr, "Unable to find a route to '%s'\n",
				Net_PrintAddress(type, addr)
				);
			return -1;
		}
		
		printf("iface = '%s'\n", iface);
	}
	
	{
		char	*_iface = malloc( sizeof("/Devices/ip/") + strlen(iface) + 1 );
		strcpy(_iface, "/Devices/ip/");
		strcat(_iface, iface);
		free(iface);	// TODO: Handle when this is not heap
		iface = _iface;
		printf("iface = '%s'\n", iface);
	}
	
	fd = open(iface, OPENFLAG_EXEC);
	if(fd == -1) {
		fprintf(stderr, "ERROR: Unable to open interface '%s'\n", iface);
		return 1;
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

/**
 * \brief Read an IPv4 Address
 */
int GetAddress4(char *String, uint8_t *Addr)
{
	 int	i = 0;
	 int	j;
	 int	val;
	
	for( j = 0; String[i] && j < 4; j ++ )
	{
		val = 0;
		for( ; String[i] && String[i] != '.'; i++ )
		{
			if('0' > String[i] || String[i] > '9') {
				printf("0<c<9 expected, '%c' found\n", String[i]);
				return 0;
			}
			val = val*10 + String[i] - '0';
		}
		if(val > 255) {
			printf("val > 255 (%i)\n", val);
			return 0;
		}
		Addr[j] = val;
		
		if(String[i] == '.')
			i ++;
	}
	if( j != 4 ) {
		printf("4 parts expected, %i found\n", j);
		return 0;
	}
	if(String[i] != '\0') {
		printf("EOS != '\\0', '%c'\n", String[i]);
		return 0;
	}
	return 1;
}

/**
 * \brief Read an IPv6 Address
 */
int GetAddress6(char *String, uint8_t *Addr)
{
	 int	i = 0;
	 int	j, k;
	 int	val, split = -1, end;
	uint16_t	hi[8], low[8];
	
	for( j = 0; String[i] && j < 8; j ++ )
	{
		if(String[i] == ':') {
			if(split != -1) {
				printf("Two '::'s\n");
				return 0;
			}
			split = j;
			i ++;
			continue;
		}
		
		val = 0;
		for( k = 0; String[i] && String[i] != ':'; i++, k++ )
		{
			val *= 16;
			if('0' <= String[i] && String[i] <= '9')
				val += String[i] - '0';
			else if('A' <= String[i] && String[i] <= 'F')
				val += String[i] - 'A' + 10;
			else if('a' <= String[i] && String[i] <= 'f')
				val += String[i] - 'a' + 10;
			else {
				printf("%c unexpected\n", String[i]);
				return 0;
			}
		}
		
		if(val > 0xFFFF) {
			printf("val (0x%x) > 0xFFFF\n", val);
			return 0;
		}
		
		if(split == -1)
			hi[j] = val;
		else
			low[j-split] = val;
		
		if( String[i] == ':' ) {
			i ++;
		}
	}
	end = j;
	
	for( j = 0; j < split; j ++ )
	{
		Addr[j*2] = hi[j]>>8;
		Addr[j*2+1] = hi[j]&0xFF;
	}
	for( ; j < 8 - (end - split); j++ )
	{
		Addr[j*2] = 0;
		Addr[j*2+1] = 0;
	}
	k = 0;
	for( ; j < 8; j ++, k++)
	{
		Addr[j*2] = hi[k]>>8;
		Addr[j*2+1] = hi[k]&0xFF;
	}
	
	return 1;
}

int GetAddress(char *String, uint8_t *Addr)
{
	if( GetAddress4(String, Addr) )
		return 4;
	
	if( GetAddress6(String, Addr) )
		return 6;
	
	return 0;
}
