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
		fd = _SysOpen("/Devices/ip/routes", 0);
		if( !fd ) {
			fprintf(stderr, "ERROR: Unable to open '/Devices/ip/routes'\n");
			return NULL;
		}
		
		// Make structure and ask
		*type = AddressType;
		memcpy(type+1, Address, size);
		routeNum = _SysIOCtl(fd, _SysIOCtl(fd, 3, "locate_route"), buf);
		
		// Close
		_SysClose(fd);
	}
	
	// Check answer validity
	if( routeNum > 0 ) {
		 int	len = sprintf(NULL, "/Devices/ip/routes/#%i", routeNum);
		char	buf[len+1];
		char	*ret;
		
		sprintf(buf, "/Devices/ip/routes/#%i", routeNum);
		
		// Open route
		fd = _SysOpen(buf, 0);
		if( fd == -1 ) {
			fprintf(stderr, "Net_GetInterface - ERROR: Unable to open %s\n", buf);
			return NULL;	// Shouldn't happen :/
		}
		
		// Allocate space for name
		ret = malloc( _SysIOCtl(fd, _SysIOCtl(fd, 3, "get_interface"), NULL) + 1 );
		if( !ret ) {
			fprintf(stderr, "malloc() failure - Allocating space for interface name\n");
			return NULL;
		}
		
		// Get name
		_SysIOCtl(fd, _SysIOCtl(fd, 3, "get_interface"), ret);
		
		// Close and return
		_SysClose(fd);
		
		return ret;
	}
	
	return NULL;	// Error
}
