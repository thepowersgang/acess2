/*
 * Acess2 IFCONFIG command
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <acess/sys.h>
#include <net.h>

// === CONSTANTS ===
#define FILENAME_MAX	255
#define IPSTACK_ROOT	"/Devices/ip"
#define DEFAULT_METRIC	30

// TODO: Move this to a header
#define ntohs(v)	(((v&0xFF)<<8)|((v>>8)&0xFF))

// === PROTOTYPES ===
void	PrintUsage(const char *ProgName);
void	DumpInterfaces(void);
void	DumpRoutes(void);
void	DumpInterface(const char *Name);
void	DumpRoute(const char *Name);
 int	AddInterface(const char *Device);
void	AddRoute(const char *Interface, int AddressType, void *Dest, int MaskBits, int Metric, void *NextHop);
 int	DoAutoConfig(const char *Device);
 int	SetAddress(int IFNum, const char *Address);
 int	ParseIPAddress(const char *Address, uint8_t *Dest, int *SubnetBits);

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
	
	// Routes
	if( strcmp(argv[1], "route") == 0 )
	{
		// Add new route
		if( argc > 2 && strcmp(argv[2], "add") == 0 )
		{
			uint8_t	dest[16] = {0};
			uint8_t	nextHop[16] = {0};
			 int	addrType, subnetBits = -1;
			 int	nextHopType, nextHopBits=-1;
			char	*ifaceName = NULL;
			 int	metric = DEFAULT_METRIC;
			// Usage:
			// ifconfig route add <host>[/<prefix>] <interface> [<metric>]
			// ifconfig route add <host>[/<prefix>] <next hop> [<metric>]
			if( argc - 3  < 2 ) {
				fprintf(stderr, "ERROR: '%s route add' takes at least two arguments, %i passed\n",
					argv[0], argc-3);
				PrintUsage(argv[0]);
				return -1;
			}
			
			if( argc - 3 > 3 ) {
				fprintf(stderr, "ERROR: '%s route add' takes at most three arguments, %i passed\n",
					argv[0], argc-3);
				PrintUsage(argv[0]);
				return -1;
			}
			
			// Destination IP
			addrType = ParseIPAddress(argv[3], dest, &subnetBits);
			if( subnetBits == -1 ) {
				subnetBits = Net_GetAddressSize(addrType)*8;
			}
			// Interface Name / Next Hop
			if( (nextHopType = ParseIPAddress(argv[4], nextHop, &nextHopBits)) == 0 )
			{
				// Interface name
				ifaceName = argv[4];
			}
			else
			{
				// Next Hop
				// - Check if it's the same type as the network/destination
				if( nextHopType != addrType ) {
					fprintf(stderr, "ERROR: Address type mismatch\n");
					return -1;
				}
				// - Make sure there's no mask
				if( nextHopBits != -1 ) {
					fprintf(stderr, "Error: Next hop cannot be masked\n");
					return -1;
				}
			}
			
			// Metric
			if( argc - 3 >= 3 )
			{
				metric = atoi(argv[5]);
				if( metric == 0 && argv[5][0] != '0' ) {
					fprintf(stderr, "ERROR: Metric should be a number\n");
					return -1;
				}
			}
			
			// Make the route!
			AddRoute(ifaceName, addrType, dest, subnetBits, metric, nextHop);
			
			return 0;
		}
		// Delete a route
		else if( argc > 2 && strcmp(argv[2], "del") == 0 )
		{
			// Usage:
			// ifconfig route del <routenum>
			// ifconfig route del <host>[/<prefix>]
		}
		else
		{
			// List routes
			DumpRoutes();
		}
		return 0;
	}
	// Add a new interface
	else if( strcmp(argv[1], "add") == 0 )
	{
		if( argc < 4 ) {
			fprintf(stderr, "ERROR: '%s add' requires two arguments, %i passed\n", argv[0], argc-2);
			PrintUsage(argv[0]);
			return -1;
		}
		ret = AddInterface( argv[2] );
		if(ret < 0)	return ret;
		ret = SetAddress( ret, argv[3] );
		return ret;
	}
	// Delete an interface
	else if( strcmp(argv[1], "del") == 0 )
	{
		if( argc < 3 ) {
			fprintf(stderr, "ERROR: '%s del' requires an argument\n", argv[0]);
			PrintUsage(argv[0]);
			return -1;
		}
		// TODO:
	}
	// Autoconfigure an interface
	// NOTE: Debugging hack (see the function for more details)
	else if( strcmp(argv[1], "autoconf") == 0 )
	{
		DoAutoConfig(argv[2]);
		return 0;
	}
	else if( strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 )
	{
		PrintUsage(argv[0]);
		return 0;
	}
	
	// Dump a named interface
	DumpInterface(argv[1]);
	
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
	fprintf(stderr, "        Delete an interface\n");
	fprintf(stderr, "    %s [<interface>]\n", ProgName);
	fprintf(stderr, "        Print the current interfaces (or only <interface> if passed)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "    %s route\n", ProgName);
	fprintf(stderr, "        Print the routing tables\n");
	fprintf(stderr, "    %s route add <host>[/<prefix>] [<nexthop> OR <iface>] [<metric>]\n", ProgName);
	fprintf(stderr, "        Add a new route\n");
	fprintf(stderr, "    %s route del <host>[/<prefix>]\n", ProgName);
	fprintf(stderr, "    %s route del <routenum>\n", ProgName);
	fprintf(stderr, "        Add a new route\n");
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
 * \brief Dump all interfaces
 */
void DumpRoutes(void)
{
	 int	dp;
	char	filename[FILENAME_MAX+1];
	
	dp = open(IPSTACK_ROOT"/routes", OPENFLAG_READ);
	
	printf("Type\tNetwork \tGateway \tMetric\tIFace\n");
	
	while( readdir(dp, filename) )
	{
		if(filename[0] == '.')	continue;
		DumpRoute(filename);
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
		fprintf(stderr, "Bad interface name '%s' (%s does not exist)\t", Name, path);
		return ;
	}
	
	type = ioctl(fd, 4, NULL);
	
	// Ignore -1 values
	if( type == -1 ) {
		return ;
	}
	
	printf("%s:\t", Name);
	{
		 int	call_num = ioctl(fd, 3, "get_device");
		 int	len = ioctl(fd, call_num, NULL);
		char	*buf = malloc(len+1);
		ioctl(fd, call_num, buf);
		printf("'%s'\n", buf);
		free(buf);
	}
	printf("\t");
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
		printf("IPv4\t");
		ioctl(fd, 5, ip);	// Get IP Address
		subnet = ioctl(fd, 7, NULL);	// Get Subnet Bits
		printf("%i.%i.%i.%i/%i\n", ip[0], ip[1], ip[2], ip[3], subnet);
		}
		break;
	case 6:	// IPv6
		{
		uint16_t	ip[8];
		 int	subnet;
		printf("IPv6\t");
		ioctl(fd, 5, ip);	// Get IP Address
		subnet = ioctl(fd, 7, NULL);	// Get Subnet Bits
		printf("%x:%x:%x:%x:%x:%x:%x:%x/%i\n",
			ntohs(ip[0]), ntohs(ip[1]), ntohs(ip[2]), ntohs(ip[3]),
			ntohs(ip[4]), ntohs(ip[5]), ntohs(ip[6]), ntohs(ip[7]),
			subnet);
		}
		break;
	default:	// Unknow
		printf("UNKNOWN (%i)\n", type);
		break;
	}
			
	close(fd);
}


/**
 * \brief Dump a route
 */
void DumpRoute(const char *Name)
{
	 int	fd;
	 int	type;
	char	path[sizeof(IPSTACK_ROOT)+8+FILENAME_MAX+1] = IPSTACK_ROOT"/routes/";
	
	strcat(path, Name);
	
	fd = open(path, OPENFLAG_READ);
	if(fd == -1) {
		printf("%s:\tUnable to open ('%s')\n", Name, path);
		return ;
	}
	
	 int	ofs = 2;
	type = atoi(Name);
	
	 int	i;
	 int	len = Net_GetAddressSize(type);
	uint8_t	net[len], gw[len];
	 int	subnet, metric;
	for( i = 0; i < len; i ++ ) {
		char tmp[5] = "0x00";
		tmp[2] = Name[ofs++];
		tmp[3] = Name[ofs++];
		net[i] = atoi(tmp);
	}
	ofs ++;
	subnet = atoi(Name+ofs);
	ofs ++;
	metric = atoi(Name+ofs);
	ioctl(fd, ioctl(fd, 3, "get_nexthop"), gw);	// Get Gateway/NextHop
	
	// Get the address type
	switch(type)
	{
	case 0:	// Disabled/Unset
		printf("DISABLED\n");
		break;
	case 4:	// IPv4
		printf("IPv4\t");
		break;
	case 6:	// IPv6
		printf("IPv6\t");
		break;
	default:	// Unknow
		printf("UNKNOWN (%i)\n", type);
		break;
	}
	printf("%s/%i\t", Net_PrintAddress(type, net), subnet);
	printf("%s \t", Net_PrintAddress(type, gw));
	printf("%i\t", metric);
	
	// Interface
	{
		 int	call_num = ioctl(fd, 3, "get_interface");
		 int	len = ioctl(fd, call_num, NULL);
		char	*buf = malloc(len+1);
		ioctl(fd, call_num, buf);
		printf("'%s'\t", buf);
		free(buf);
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

void AddRoute(const char *Interface, int AddressType, void *Dest, int MaskBits, int Metric, void *NextHop)
{
	 int	fd;
	 int	num;
	char	*ifaceToFree = NULL;
	
	// Get interface name
	if( !Interface )
	{
		if( !NextHop ) {
			fprintf(stderr,
				"BUG: AddRoute(Interface=NULL,...,NextHop=NULL)\n"
				"Only one should be NULL\n"
				);
			return ;
		}
		
		// Query for the interface name
		Interface = ifaceToFree = Net_GetInterface(AddressType, NextHop);
	}
	// Check address type (if the interface was passed)
	// - If we got the interface name, then it should be correct
	else
	{
		char	ifacePath[sizeof(IPSTACK_ROOT"/")+strlen(Interface)+1];
		
		// Open interface
		strcpy(ifacePath, IPSTACK_ROOT"/");
		strcat(ifacePath, Interface);
		fd = open(ifacePath, 0);
		if( fd == -1 ) {
			fprintf(stderr, "Error: Interface '%s' does not exist\n", Interface);
			return ;
		}
		// Get and check type
		num = ioctl(fd, ioctl(fd, 3, "getset_type"), NULL);
		if( num != AddressType ) {
			fprintf(stderr, "Error: Passed type does not match interface type (%i != %i)\n",
				AddressType, num);
			return ;
		}
		
		close(fd);
	}
	
	// Create route
	 int	addrsize = Net_GetAddressSize(AddressType);
	 int	len = snprintf(NULL, 0, "/Devices/ip/routes/%i::%i:%i", AddressType, MaskBits, Metric) + addrsize*2;
	char	path[len+1];
	{
		 int	i, ofs;
		ofs = sprintf(path, "/Devices/ip/routes/%i:", AddressType);
		for( i = 0; i < addrsize; i ++ )
			sprintf(path+ofs+i*2, "%02x", ((uint8_t*)Dest)[i]);
		ofs += addrsize*2;
		sprintf(path+ofs, ":%i:%i", MaskBits, Metric);
	}

	fd = open(path, 0);
	if( fd != -1 ) {
		close(fd);
		fprintf(stderr, "Unable to create route '%s', already exists\n", path);
		return ;
	}
	fd = open(path, OPENFLAG_CREATE, 0);
	if( fd == -1 ) {
		fprintf(stderr, "Unable to create '%s'\n", path);
		return ;
	}
	
	if( NextHop )
		ioctl(fd, ioctl(fd, 3, "set_nexthop"), NextHop);
	ioctl(fd, ioctl(fd, 3, "set_interface"), (void*)Interface);
	
	close(fd);
	
	// Check if the interface name was allocated by us
	if( ifaceToFree )
		free(ifaceToFree);
}

/**
 * \note Debugging HACK!
 * \brief Autoconfigure the specified device to 10.0.2.55/24 using
 *        10.0.2.2 as the gateway.
 */
int DoAutoConfig(const char *Device)
{
	 int	tmp, fd;
	char	path[sizeof(IPSTACK_ROOT)+1+4+1];	// /0000
	uint8_t	addr[4] = {10,0,2,55};
	uint8_t	gw[4] = {10,0,2,2};
	 int	subnet = 24;
	
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
	
	// Set routes
	{
		uint8_t	net[4] = {0,0,0,0};
		AddRoute(path + sizeof(IPSTACK_ROOT), 4, addr, subnet, DEFAULT_METRIC, net);	// This interface
		AddRoute(path + sizeof(IPSTACK_ROOT), 4, net, 0, DEFAULT_METRIC, gw);	// Gateway
	}
	
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
	type = ParseIPAddress(Address, addr, &subnet);
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
int ParseIPAddress(const char *Address, uint8_t *Dest, int *SubnetBits)
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
			if(SubnetBits)	*SubnetBits = val;
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
			if(SubnetBits)	*SubnetBits = val;
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
