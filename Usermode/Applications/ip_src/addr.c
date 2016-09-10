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
 int	AddInterface(const char *Device, int Type);
 int	SetAddress(int IFNum, const void *Address, int SubnetSize);
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
		uint8_t	addr[16];
		 int	type, subnet_size;

		// Check argument counts
		if( argc - 2 < 2 ) {
			fprintf(stderr, "ERROR: '%s add' requires two arguments, %i passed\n",
				argv[0], argc-2);
			PrintUsage(argv[0]);
			return -1;
		}

		// Parse IP Address
		type = ParseIPAddress(argv[3], addr, &subnet_size);
		if(type == 0) {
			fprintf(stderr, "'%s' cannot be parsed as an IP address\n", argv[3]);
			return -1;
		}
	
		// Fun
		ret = AddInterface( argv[2], type );
		if(ret < 0)	return ret;
		ret = SetAddress( ret, addr, subnet_size );
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
int AddInterface(const char *Device, int Type)
{
	 int	dp, ret;
	
	dp = _SysOpen(IPSTACK_ROOT, OPENFLAG_READ);
	struct {
		const char *Dev;
		const char *Name;
		 int	Type;
	} ifinfo;
	ifinfo.Dev = Device;
	ifinfo.Name = "";
	ifinfo.Type = Type;
	ret = _SysIOCtl(dp, 4, &ifinfo);
	_SysClose(dp);
	
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
int SetAddress(int IFNum, const void *Address, int SubnetSize)
{
	char	path[sizeof(IPSTACK_ROOT)+1+5+1];	// ip000
	 int	fd;
	
	// Open file
	sprintf(path, IPSTACK_ROOT"/%i", IFNum);
	fd = _SysOpen(path, OPENFLAG_READ);
	if( fd == -1 ) {
		fprintf(stderr, "Unable to open '%s'\n", path);
		return -1;
	}
	
	// Set Address
	_SysIOCtl(fd, _SysIOCtl(fd, 3, "set_address"), (void*)Address);
	
	// Set Subnet
	_SysIOCtl(fd, _SysIOCtl(fd, 3, "getset_subnet"), &SubnetSize);
	
	_SysClose(fd);
	
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
	
	dp = _SysOpen(IPSTACK_ROOT, OPENFLAG_READ);
	
	while( _SysReadDir(dp, filename) )
	{
		if(filename[0] == '.')	continue;
		DumpInterface(filename);
	}
	
	_SysClose(dp);
}

/**
 * \brief Dump an interface
 */
void DumpInterface(const char *Name)
{
	 int	fd;
	 int	type;
	char	path[sizeof(IPSTACK_ROOT)+1+FILENAME_MAX+1] = IPSTACK_ROOT"/";
	
	if(strlen(Name) + 1 > sizeof(path) - strlen(path)) {
		fprintf(stderr, "Bad interface name '%s' (name too long)\n", Name);
		return;
	}
	
	strcat(path, Name);
	
	fd = _SysOpen(path, OPENFLAG_READ);
	if(fd == -1) {
		fprintf(stderr, "Bad interface name '%s' (%s does not exist)\t", Name, path);
		return ;
	}
	
	type = _SysIOCtl(fd, 4, NULL);
	
	// Ignore -1 values
	if( type == -1 ) {
		return ;
	}
	
	printf("%s:\t", Name);
	{
		 int	call_num = _SysIOCtl(fd, 3, "get_device");
		 int	len = _SysIOCtl(fd, call_num, NULL);
		char	*buf = malloc(len+1);
		_SysIOCtl(fd, call_num, buf);
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
		_SysIOCtl(fd, 5, ip);	// Get IP Address
		subnet = _SysIOCtl(fd, 7, NULL);	// Get Subnet Bits
		printf("%i.%i.%i.%i/%i\n", ip[0], ip[1], ip[2], ip[3], subnet);
		}
		break;
	case 6:	// IPv6
		{
		uint16_t	ip[8];
		 int	subnet;
		printf("IPv6\t");
		_SysIOCtl(fd, 5, ip);	// Get IP Address
		subnet = _SysIOCtl(fd, 7, NULL);	// Get Subnet Bits
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
			
	_SysClose(fd);
}

