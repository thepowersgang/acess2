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

int Net_OpenSocket_TCPC(int AddrType, void *Addr, int Port)
{
	int fd = Net_OpenSocket(AddrType, Addr, "tcpc");
	if( fd == -1 )	return -1;
	
	_SysIOCtl(fd, 5, &Port);	// Remote Port
        _SysIOCtl(fd, 6, Addr);	// Remote address
	_SysIOCtl(fd, 7, NULL);	// connect
	return fd;
}

