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
void	PrintUsage(const char *ProgName);
void	DumpInterfaces(void);
void	DumpInterface(const char *Name);
 int	AddInterface(const char *Device);
 int	DoAutoConfig(const char *Device);

// === CODE ===
/**
 * \brief Program entrypoint
 */
int main(int argc, char *argv[])
{
	// No args, dump interfaces
	if(argc == 1) {
		DumpInterfaces();
		return 0;
	}
	
	// Add a new interface
	if( strcmp(argv[1], "add") == 0 ) {
		if( argc < 4 ) {
			fprintf(stderr, "ERROR: %s add require two arguments, %i passed\n", argv[0], argc-2);
			PrintUsage(argv[0]);
			return 0;
		}
		// TODO: Also set the IP address as the usage says it does
		return AddInterface( argv[2] );
	}
	
	// Autoconfigure an interface
	// NOTE: Debugging hack (see the function for more details)
	if( strcmp(argv[1], "autoconf") == 0 ) {
		DoAutoConfig(argv[2]);
		return 0;
	}
	
	// Print usage instructions
	PrintUsage(argv[0]);
	
	return 0;
}

/**
 * \brief Print usage instructions
 */
void PrintUsage(const char *ProgName)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "    %s add <device> <ip>/<prefix>\n", ProgName);
	fprintf(stderr, "        Add a new interface listening on <device> with the specified\n");
	fprintf(stderr, "        address.\n");
	fprintf(stderr, "    %s del <interface>\n", ProgName);
	fprintf(stderr, "    %s set <interface> <option> <value>\n", ProgName);
	fprintf(stderr, "        Set an option on an interface, a list of valid options follows\n");
	fprintf(stderr, "        gw      IPv4 default gateway\n");
	fprintf(stderr, "    %s [<interface>]\n", ProgName);
	fprintf(stderr, "        Print the current interfaces (or only <interface> if passed)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "A note on Acess's IP Stack:\n");
	fprintf(stderr, "    Each interface corresponds to only one IP address (either IPv4\n");
	fprintf(stderr, "    or IPv6). A network device can have multiple interfaces bound\n");
	fprintf(stderr, "    to it, allowing multiple addresses for one network connection\n");
	fprintf(stderr, "\n");
}

/**
 * \brief Dump all interfaces
 */
void DumpInterfaces(void)
{
	 int	dp;
	char	filename[FILENAME_MAX+1];
	
	dp = open(IPSTACK_ROOT, OPENFLAG_READ);
	
	while( readdir(dp, filename) )
	{
		if(filename[0] == '.')	continue;
		DumpInterface(filename);
	}
	
	close(dp);
}

/**
 * \brief Dump an interface
 */
void DumpInterface(const char *Name)
{
	 int	fd;
	 int	type;
	char	path[sizeof(IPSTACK_ROOT)+1+FILENAME_MAX+1] = IPSTACK_ROOT"/";
	
	strcat(path, Name);
	
	fd = open(path, OPENFLAG_READ);
	if(fd == -1) {
		printf("%s:\tUnable to open ('%s')\n", Name, path);
		return ;
	}
	
	type = ioctl(fd, 4, NULL);
	
	printf("%s:\t", Name);
	{
		 int	call_num = ioctl(fd, 3, "get_device");
		 int	len = ioctl(fd, call_num, NULL);
		char	*buf = malloc(len+1);
		ioctl(fd, call_num, buf);
		printf("'%s'\t", buf);
		free(buf);
	}
	// Get the address type
	switch(type)
	{
	case 0:	// Disabled/Unset
		printf("DISABLED\n");
		break;
	case 4:	// IPv4
		{
		uint8_t	ip[4];
		 int	subnet;
		printf("IPv4\n");
		ioctl(fd, 5, ip);	// Get IP Address
		subnet = ioctl(fd, 7, NULL);	// Get Subnet Bits
		printf("\tAddress: %i.%i.%i.%i/%i\n", ip[0], ip[1], ip[2], ip[3], subnet);
		ioctl(fd, 8, ip);	// Get Gateway
		printf("\tGateway: %i.%i.%i.%i\n", ip[0], ip[1], ip[2], ip[3]);
		}
		break;
	case 6:	// IPv6
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
	default:	// Unknow
		printf("UNKNOWN (%i)\n", type);
		break;
	}
	printf("\n");
			
	close(fd);
}

/**
 * \brief Create a new interface using the passed device
 * \param Device	Network device to bind to
 */
int AddInterface(const char *Device)
{
	 int	dp, ret;
	
	dp = open(IPSTACK_ROOT, OPENFLAG_READ);
	ret = ioctl(dp, 4, (void*)Device);
	printf("AddInterface: ret = 0x%x = %i\n", ret, ret);
	close(dp);
	
	if( ret < 0 ) {
		fprintf(stderr, "Unable to add '%s' as a network interface\n", Device);
		return -1;
	}
	
	printf("-- Added '"IPSTACK_ROOT"/%i' using device %s\n", ret, Device);
	
	return ret;
}

/**
 * \note Debugging HACK!
 * \brief Autoconfigure the specified device to 10.0.2.55/8 using
 *        10.0.2.1 as the gateway.
 */
int DoAutoConfig(const char *Device)
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
