/*
 * Acess2 Networking Toolkit
 * By John Hodge (thePowersGang)
 * 
 * socket.c
 * - 
 */
#include <net.h>
#include <stdio.h>
#include <stdint.h>
#include <acess/sys.h>

enum {
	UDP_IOCTL_GETSETLPORT = 4,
	UDP_IOCTL_GETSETRPORT,
	UDP_IOCTL_GETSETRMASK,
	UDP_IOCTL_SETRADDR,
};

int Net_OpenSocket(int AddrType, const void *Addr, const char *Filename)
{
	 int	addrLen = Net_GetAddressSize(AddrType);
	char	hexAddr[addrLen*2+1];
	
	{
		const uint8_t	*addrBuffer = Addr;
		for( unsigned int i = 0; i < addrLen; i ++ )
			sprintf(hexAddr+i*2, "%02x", addrBuffer[i]);
	}
	
	if(Filename)
	{
		 int	len = snprintf(NULL, 0, "/Devices/ip/routes/@%i:%s/%s", AddrType, hexAddr, Filename);
		char	path[len+1];
		snprintf(path, len+1, "/Devices/ip/routes/@%i:%s/%s", AddrType, hexAddr, Filename);
		_SysDebug("%s", path);
		return _SysOpen(path, OPENFLAG_READ|OPENFLAG_WRITE);
	}
	else
	{
		 int	len = snprintf(NULL, 0, "/Devices/ip/routes/@%i:%s", AddrType, hexAddr);
		char	path[len+1];
		snprintf(path, len+1, "/Devices/ip/routes/@%i:%s", AddrType, hexAddr);
		return _SysOpen(path, OPENFLAG_READ);
	}
}

int Net_OpenSocket_TCPC(int AddrType, const void *Addr, int Port)
{
	int fd = Net_OpenSocket(AddrType, Addr, "tcpc");
	if( fd == -1 )	return -1;
	
	_SysIOCtl(fd, 5, &Port);	// Remote Port
        _SysIOCtl(fd, 6, (void*)Addr);	// Remote address (kernel shouldn't modify)
	_SysIOCtl(fd, 7, NULL);	// connect
	return fd;
}

int Net_OpenSocket_UDP(int AddrType, const void *Addr, int RPort, int LPort)
{
	int fd = Net_OpenSocket(AddrType, Addr, "udp");
	if( fd == -1 )	return -1;
	
	_SysIOCtl(fd, UDP_IOCTL_GETSETLPORT, &LPort);	// Remote Port
	int maskbits = Net_GetAddressSize(AddrType) * 8;
	_SysIOCtl(fd, UDP_IOCTL_GETSETRPORT, &RPort);
	_SysIOCtl(fd, UDP_IOCTL_GETSETRMASK, &maskbits);
        _SysIOCtl(fd, UDP_IOCTL_SETRADDR, (void*)Addr);	// Remote address (kernel shouldn't modify)
	return fd;
}

