/*
 * Acess2 IFCONFIG command
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <acess/sys.h>

// === CONSTANTS ===
#define FILENAME_MAX	255
#define IPSTACK_ROOT	"/Devices/ip"

// TODO: Move this to a header
#define ntohs(v)	(((v&0xFF)<<8)|((v>>8)&0xFF))

// === PROTOTYPES ===
void	PrintUsage(const char *ProgName);
void	DumpInterfaces(void);
void	DumpInterface(const char *Name);
 int	AddInterface(const char *Device);
 int	DoAutoConfig(const char *Device);
 int	SetAddress(int IFNum, const char *Address);
 int	ParseIPAddres(const char *Address, uint8_t *Dest, int *SubnetBits);

// === CODE ===
/**
 * \brief Program entrypoint
 */
int main(int argc, char *argv[])
{
	 int	ret;
	// No args, dump interfaces
	if(argc == 1) {
		DumpInterfaces();
		return 0;
	}
	
	// Add a new interface
	if( strcmp(argv[1], "add") == 0 ) {
		if( argc < 4 ) {
			fprintf(stderr, "ERROR: '%s add' requires two arguments, %i passed\n", argv[0], argc-2);
			PrintUsage(argv[0]);
			return -1;
		}
		// TODO: Also set the IP address as the usage says it does
		ret = AddInterface( argv[2] );
		if(ret < 0)	return ret;
		ret = SetAddress( ret, argv[3] );
		return ret;
	}
	
	// Delete an interface
	if( strcmp(argv[1], "del") == 0 ) {
		if( argc < 3 ) {
			fprintf(stderr, "ERROR: '%s del' requires an argument\n", argv[0]);
			PrintUsage(argv[0]);
			return -1;
		}
		// TODO:
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
			ntohs(ip[0]), ntohs(ip[1]), ntohs(ip[2]), ntohs(ip[3]),
			ntohs(ip[4]), ntohs(ip[5]), ntohs(ip[6]), ntohs(ip[7]),
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

/**
 * \brief Set the address on an interface from a textual IP address
 */
int	SetAddress(int IFNum, const char *Address)
{
	uint8_t	addr[16];
	 int	type;
	char	path[sizeof(IPSTACK_ROOT)+1+5+1];	// ip000
	 int	tmp, fd, subnet;
	
	// Parse IP Address
	type = ParseIPAddres(Address, addr, &subnet);
	if(type == 0) {
		fprintf(stderr, "'%s' cannot be parsed as an IP address\n", Address);
		return -1;
	}
	
	// Open file
	sprintf(path, IPSTACK_ROOT"/%i", IFNum);
	fd = open(path, OPENFLAG_READ);
	if( fd == -1 ) {
		fprintf(stderr, "Unable to open '%s'\n", path);
		return -1;
	}
	
	tmp = type;
	tmp = ioctl(fd, ioctl(fd, 3, "getset_type"), &tmp);
	if( tmp != type ) {
		fprintf(stderr, "Error in setting address type (got %i, expected %i)\n", tmp, type);
		close(fd);
		return -1;
	}
	// Set Address
	ioctl(fd, ioctl(fd, 3, "set_address"), addr);
	
	// Set Subnet
	ioctl(fd, ioctl(fd, 3, "getset_subnet"), &subnet);
	
	close(fd);
	
	// Dump!
	//DumpInterface( path+sizeof(IPSTACK_ROOT)+1 );
	
	return 0;
}

/**
 * \brief Parse an IP Address
 * \return 0 for unknown, 4 for IPv4 and 6 for IPv6
 */
int ParseIPAddres(const char *Address, uint8_t *Dest, int *SubnetBits)
{
	const char	*p = Address;
	
	// Check first block
	while(*p && *p >= '0' && *p <= '9')	p ++;
	
	// IPv4?
	if(*p == '.')
	{
		 int	i = 0, j;
		 int	val;
		
		for( j = 0; Address[i] && j < 4; j ++ )
		{
			val = 0;
			for( ; '0' <= Address[i] && Address[i] <= '9'; i++ )
			{
				val = val*10 + Address[i] - '0';
			}
			if(val > 255) {
				//printf("val > 255 (%i)\n", val);
				return 0;
			}
			Dest[j] = val;
			
			if(Address[i] == '.')
				i ++;
		}
		if( j != 4 ) {
			//printf("4 parts expected, %i found\n", j);
			return 0;
		}
		// Parse subnet size
		if(Address[i] == '/') {
			val = 0;
			i ++;
			while('0' <= Address[i] && Address[i] <= '9') {
				val *= 10;
				val += Address[i] - '0';
				i ++;
			}
			if(val > 32) {
				printf("Notice: Subnet size >32 (%i)\n", val);
			}
			*SubnetBits = val;
		}
		if(Address[i] != '\0') {
			//printf("EOS != '\\0', '%c'\n", Address[i]);
			return 0;
		}
		return 4;
	}
	
	// IPv6
	if(*p == ':' || ('a' <= *p && *p <= 'f') || ('A' <= *p && *p <= 'F'))
	{
		 int	i = 0;
		 int	j, k;
		 int	val, split = -1, end;
		uint16_t	hi[8], low[8];
		
		for( j = 0; Address[i] && j < 8; j ++ )
		{
			if(Address[i] == '/')
				break;
			
			if(Address[i] == ':') {
				if(split != -1) {
					printf("Two '::'s\n");
					return 0;
				}
				split = j;
				i ++;
				continue;
			}
			
			val = 0;
			for( k = 0; Address[i] && Address[i] != ':' && Address[i] != '/'; i++, k++ )
			{
				val *= 16;
				if('0' <= Address[i] && Address[i] <= '9')
					val += Address[i] - '0';
				else if('A' <= Address[i] && Address[i] <= 'F')
					val += Address[i] - 'A' + 10;
				else if('a' <= Address[i] && Address[i] <= 'f')
					val += Address[i] - 'a' + 10;
				else {
					printf("%c unexpected\n", Address[i]);
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
			
			if( Address[i] == ':' ) {
				i ++;
			}
		}
		end = j;
		
		// Parse subnet size
		if(Address[i] == '/') {
			val = 0;
			while('0' <= Address[i] && Address[i] <= '9') {
				val *= 10;
				val += Address[i] - '0';
				i ++;
			}
			if(val > 128) {
				printf("Notice: Subnet size >128 (%i)\n", val);
			}
			*SubnetBits = val;
		}
		
		for( j = 0; j < split; j ++ )
		{
			//printf("%04x:", hi[j]);
			Dest[j*2] = hi[j]>>8;
			Dest[j*2+1] = hi[j]&0xFF;
		}
		for( ; j < 8 - (end - split); j++ )
		{
			//printf("0000:", hi[j]);
			Dest[j*2] = 0;
			Dest[j*2+1] = 0;
		}
		for( k = 0; j < 8; j ++, k++)
		{
			//printf("%04x:", low[k]);
			Dest[j*2] = low[k]>>8;
			Dest[j*2+1] = low[k]&0xFF;
		}
		return 6;
	}
	// Unknown type
	return 0;
}
