/*
 * Acess2 Networking Toolkit
 * By John Hodge (thePowersGang)
 * 
 * main.c
 * - Library main (and misc functions)
 */
#include <net.h>
#include <acess/sys.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// === CODE ===
int SoMain(void)
{
	return 0;
}

int Net_GetAddressSize(int AddressType)
{
	switch(AddressType)
	{
	case 4:	return 4;	// IPv4
	case 6:	return 16;	// IPv6
	default:
		return 0;
	}
}

int Net_OpenSocket(int AddrType, void *Addr, const char *Filename)
{
	 int	addrLen = Net_GetAddressSize(AddrType);
	 int	i;
	uint8_t	*addrBuffer = Addr;
	char	hexAddr[addrLen*2+1];
	
	for( i = 0; i < addrLen; i ++ )
		sprintf(hexAddr+i*2, "%02x", addrBuffer[i]);
	
	if(Filename)
	{
		 int	len = snprintf(NULL, 100, "/Devices/ip/routes/%i:%s/%s", AddrType, hexAddr, Filename);
		char	path[len+1];
		snprintf(path, 100, "/Devices/ip/routes/%i:%s/%s", AddrType, hexAddr, Filename);
		return open(path, OPENFLAG_READ|OPENFLAG_WRITE);
	}
	else
	{
		 int	len = snprintf(NULL, 100, "/Devices/ip/routes/%i:%s", AddrType, hexAddr);
		char	path[len+1];
		snprintf(path, 100, "/Devices/ip/routes/%i:%s", AddrType, hexAddr);
		return open(path, OPENFLAG_READ);
	}
}

//TODO: Move out to another file
char *Net_GetInterface(int AddressType, void *Address)
{
	 int	size, routeNum;
	 int	fd;
	size = Net_GetAddressSize(AddressType);
	
	if( size == 0 ) {
		fprintf(stderr, "BUG: AddressType = %i unknown\n", AddressType);
		return NULL;
	}
	
	// Query the route manager for the route number
	{
		char	buf[sizeof(int)+size];
		uint32_t	*type = (void*)buf;
		
		// Open
		fd = open("/Devices/ip/routes", 0);
		if( !fd ) {
			fprintf(stderr, "ERROR: It seems that '/Devices/ip/routes' does not exist, are you running Acess2?\n");
			return NULL;
		}
		
		// Make structure and ask
		*type = AddressType;
		memcpy(type+1, Address, size);
		routeNum = ioctl(fd, ioctl(fd, 3, "locate_route"), buf);
		
		// Close
		close(fd);
	}
	
	// Check answer validity
	if( routeNum > 0 ) {
		 int	len = sprintf(NULL, "/Devices/ip/routes/%i", routeNum);
		char	buf[len+1];
		char	*ret;
		
		sprintf(buf, "/Devices/ip/routes/%i", routeNum);
		
		// Open route
		fd = open(buf, 0);
		if( !fd )	return NULL;	// Shouldn't happen :/
		
		// Allocate space for name
		ret = malloc( ioctl(fd, ioctl(fd, 3, "get_interface"), NULL) + 1 );
		if( !ret ) {
			fprintf(stderr, "malloc() failure - Allocating space for interface name\n");
			return NULL;
		}
		
		// Get name
		ioctl(fd, ioctl(fd, 3, "get_interface"), ret);
		
		// Close and return
		close(fd);
		
		return ret;
	}
	
	return NULL;	// Error
}
