/*
 * Acess2 IFCONFIG command
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <acess/sys.h>

// === CONSTANTS ===
#define FILENAME_MAX	255
#define IPSTACK_ROOT	"/Devices/ip"

// === PROTOTYPES ===
void	PrintUsage(char *ProgName);
void	DumpInterfaces( int DumpAll );
 int	AddInterface( char *Address );
 int	DoAutoConfig( char *Device );

// === CODE ===
/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	if(argc == 1) {
		DumpInterfaces(0);
		return 0;
	}
	
	if( strcmp(argv[1], "add") == 0 ) {
		if( argc < 3 ) {
			fprintf(stderr, "ERROR: `add` requires an argument\n");
			PrintUsage(argv[0]);
			return 0;
		}
		return AddInterface( argv[2] );
	}
	
	if( strcmp(argv[1], "autoconf") == 0 ) {
		DoAutoConfig(argv[2]);
	}
	
	return 0;
}

void PrintUsage(char *ProgName)
{
	fprintf(stderr, "Usage: %s [add <ipaddr>]\n", ProgName);
}

void DumpInterfaces(int DumpAll)
{
	 int	dp, fd;
	 int	type;
	char	path[sizeof(IPSTACK_ROOT)+1+FILENAME_MAX] = IPSTACK_ROOT"/";
	char	*filename = &path[sizeof(IPSTACK_ROOT)];
	
	dp = open(IPSTACK_ROOT, OPENFLAG_READ);
	
	while( readdir(dp, filename) )
	{
		if(filename[0] == '.')	continue;
		
		fd = open(path, OPENFLAG_READ);
		if(fd == -1) {
			printf("%s:\tUnable to open ('%s'\n\n", filename, path);
		}
		
		type = ioctl(fd, 4, NULL);
		
		printf("%s:\t", filename);
		switch(type)
		{
		case 0:
			printf("DISABLED\n");
			break;
		case 4:
			{
			uint8_t	ip[4];
			 int	subnet;
			printf("IPv4\n");
			ioctl(fd, 5, ip);	// Get IP Address
			subnet = ioctl(fd, 7, NULL);	// Get Subnet Bits
			printf("\t%i.%i.%i.%i/%i\n", ip[0], ip[1], ip[2], ip[3], subnet);
			ioctl(fd, 8, ip);	// Get Gateway
			printf("\tGateway: %i.%i.%i.%i\n", ip[0], ip[1], ip[2], ip[3]);
			}
			break;
		case 6:
			{
			uint16_t	ip[8];
			 int	subnet;
			printf("IPv6\n");
			ioctl(fd, 5, ip);	// Get IP Address
			subnet = ioctl(fd, 7, NULL);	// Get Subnet Bits
			printf("\t%x:%x:%x:%x:%x:%x:%x:%x/%i\n",
				ip[0], ip[1], ip[2], ip[3],
				ip[4], ip[5], ip[6], ip[7],
				subnet);
			}
			break;
		default:
			printf("UNKNOWN\n");
			break;
		}
		printf("\n");
			
		close(fd);
	}
	
	close(dp);
}

int AddInterface( char *Device )
{
	 int	dp, ret;
	
	dp = open(IPSTACK_ROOT, OPENFLAG_READ);
	ret = ioctl(dp, 4, Device);
	close(dp);
	
	if( ret < 0 ) {
		fprintf(stderr, "Unable to add '%s' as a network interface\n", Device);
		return -1;
	}
	
	printf("-- Added '"IPSTACK_ROOT"/%i' using device %s\n", ret, Device);
	
	return ret;
}

int DoAutoConfig( char *Device )
{
	 int	tmp, fd;
	char	path[sizeof(IPSTACK_ROOT)+5+1];	// ip000
	uint8_t	addr[4] = {10,0,2,55};
	uint8_t	gw[4] = {10,0,2,1};
	 int	subnet = 8;
	
	tmp = AddInterface(Device);
	if( tmp < 0 )	return tmp;
	
	sprintf(path, IPSTACK_ROOT"/%i", tmp);
	
	fd = open(path, OPENFLAG_READ);
	if( fd == -1 ) {
		fprintf(stderr, "Unable to open '%s'\n", path);
		return -1;
	}
	
	tmp = 4;	// IPv4
	tmp = ioctl(fd, ioctl(fd, 3, "getset_type"), &tmp);
	if( tmp != 4 ) {
		fprintf(stderr, "Error in setting address type (got %i, expected 4)\n", tmp);
		return -1;
	}
	// Set Address
	ioctl(fd, ioctl(fd, 3, "set_address"), addr);
	// Set Subnet
	ioctl(fd, ioctl(fd, 3, "getset_subnet"), &subnet);
	// Set Gateway
	ioctl(fd, ioctl(fd, 3, "set_gateway"), gw);
	
	close(fd);
	
	printf("Set address to %i.%i.%i.%i/%i (GW: %i.%i.%i.%i)\n",
		addr[0], addr[1], addr[2], addr[3],
		subnet,
		gw[0], gw[1], gw[2], gw[3]);
	
	return 0;
}
