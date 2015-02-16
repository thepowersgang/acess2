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
#include <string.h>	// memcpy
#include <acess/sys.h>

enum {
	UDP_IOCTL_GETSETLPORT = 4,
	UDP_IOCTL_GETSETRPORT,
	UDP_IOCTL_GETSETRMASK,
	UDP_IOCTL_SETRADDR,
	UDP_IOCTL_SENDTO,
	UDP_IOCTL_RECVFROM,
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
	
	if( _SysIOCtl(fd, 5, &Port) < 0 )	// Remote Port
		goto err;
	if( _SysIOCtl(fd, 6, (void*)Addr) < 0 )	// Remote address (kernel shouldn't modify)
		goto err;
	if( _SysIOCtl(fd, 7, NULL) < 0)	// connect
		goto err;
	return fd;
err:
	_SysClose(fd);
	return -1;
}

int Net_OpenSocket_UDP(int AddrType, const void *Addr, int RPort, int LPort)
{
	int fd = Net_OpenSocket(AddrType, Addr, "udp");
	if( fd == -1 )	return -1;
	
	if( _SysIOCtl(fd, UDP_IOCTL_GETSETLPORT, &LPort) < 0 )
		goto err;
	if( _SysIOCtl(fd, UDP_IOCTL_GETSETRPORT, &RPort) < 0 )
		goto err;
	int maskbits = Net_GetAddressSize(AddrType) * 8;
	if( _SysIOCtl(fd, UDP_IOCTL_GETSETRMASK, &maskbits) < 0 )
		goto err;
	if( _SysIOCtl(fd, UDP_IOCTL_SETRADDR, (void*)Addr) < 0 )	// Remote address (kernel shouldn't modify)
		goto err;
	return fd;
err:
	_SysClose(fd);
	return -1;
}

int Net_UDP_SendTo(int FD, int Port, int AddrType, const void *Addr, size_t Length, const void *Data)
{
	struct {
		uint16_t port;
		uint16_t addr_type;
		char	addr[16];
	} ep;
	ep.port = Port;
	ep.addr_type = AddrType;
	memcpy(ep.addr, Addr, Net_GetAddressSize(AddrType));
	struct {
		const void *ep;
		const void *buf;
		uint16_t len;
	} info = { .ep = &ep, .buf = Data, .len = Length };
	
	return _SysIOCtl(FD, UDP_IOCTL_SENDTO, &info);
}

int Net_UDP_RecvFrom(int FD, int* Port, int* AddrType, void *Addr, size_t Length, void *Data)
{
	struct {
		uint16_t port;
		uint16_t addr_type;
		char	addr[16];
	} ep;
	struct {
		void *ep;
		void *buf;
		uint16_t len;
	} info = { .ep = &ep, .buf = Data, .len = Length };
	
	int rv = _SysIOCtl(FD, UDP_IOCTL_RECVFROM, &info);
	if( rv > 0 )
	{
		if(Port)	*Port = ep.port;
		if(AddrType)	*AddrType = ep.addr_type;
		if(Addr)	memcpy(Addr, ep.addr, Net_GetAddressSize(ep.addr_type));
	}
	return rv;
}

