/*
 * Acess2 ip command
 * - By John Hodge (thePowersGang)
 * 
 * addr.c
 * - `ip addr` sub command
 */
#include "common.h"

// === PROTOTYPES ===
 int	Addr_main(int argc, char *argv[]);
 int	AddInterface(const char *Device);
 int	SetAddress(int IFNum, const char *Address);
void	DumpInterfaces(void);
void	DumpInterface(const char *Name);
void	DumpRoute(const char *Name);

// === CODE ===
int Addr_main(int argc, char *argv[])
{
	 int	ret;
	if( argc <= 1 )
	{
		// No action
		DumpInterfaces();
	}
	else if( strcmp(argv[1], "add") == 0 )
	{
		if( argc - 2 < 2 ) {
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
		if( argc - 2 < 1 ) {
			fprintf(stderr, "ERROR: '%s del' requires an argument\n", argv[0]);
			PrintUsage(argv[0]);
			return -1;
		}
	}
	else
	{
		// Show named iface?
		DumpInterface(argv[1]);
	}
	return 0;
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
 * \brief Set the address on an interface from a textual IP address
 */
int SetAddress(int IFNum, const char *Address)
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
 * \brief Dump all interfaces
 */
void DumpInterfaces(void)
{
	 int	dp;
	char	filename[FILENAME_MAX+1];
	
	dp = open(IPSTACK_ROOT, OPENFLAG_READ);
	
	while( SysReadDir(dp, filename) )
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

