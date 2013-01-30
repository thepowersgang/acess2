/*
 * Acess2 Networking Test Suite (NetTest)
 * - By John Hodge (thePowersGang)
 *
 * helpers.c
 * - General purpose helper functions
 */
#include <acess.h>
#include <vfs.h>
#include <IPStack/ipstack.h>
#include <IPStack/interface.h>

// === IMPORTS ===
//extern tInterface	*IPStack_AddInterface(const char *Device, int Type, const char *Name);
extern tVFS_Node	*IPStack_Root_FindDir(tVFS_Node *Node, const char *Name);

// === PROTOTYES ===
 int	Net_ParseAddress(const char *String, void *Addr);

// === CODE ===
int NetTest_AddAddress(char *SetAddrString)
{
	uint8_t	addr[16];
	 int	addrtype, netmask;
	char	*ifend, *addrend, *end;

	// SetAddrString:
	// <interface>,<ip>/<netmask>
	ifend = strchr(SetAddrString, ',');
	*ifend = '\0';
	addrend = strchr(ifend+1, '/');
	*addrend = '\0';

	addrtype = Net_ParseAddress(ifend+1, addr);
	netmask = strtol(addrend+1, &end, 10);

	*ifend = ',';
	*addrend = '/';

	if( *end != '\0' || addrtype == 0 )
		return 1;

	// Create interface
	*ifend = '\0';
	tInterface *iface = IPStack_AddInterface(SetAddrString, addrtype, "");
	*ifend = ',';
	if( !iface ) {
		return 1;
	}

	// Set interface address
	iface->Node.Type->IOCtl(&iface->Node, 6, addr);
	iface->Node.Type->IOCtl(&iface->Node, 7, &netmask);
//	iface->Node.Type->Close(&iface->Node);

	return 0;
}

// ----------------------------------
// Copy-pasta'd from userland libnet
// ----------------------------------
/**
 * \brief Read an IPv4 Address
 * \param String	IPv4 dotted decimal address
 * \param Addr	Output 32-bit representation of IP address
 * \return Boolean success
 */
static int Net_ParseIPv4Addr(const char *String, uint8_t *Addr)
{
	 int	i = 0;
	 int	j;
	 int	val;
	
	for( j = 0; String[i] && j < 4; j ++ )
	{
		val = 0;
		for( ; String[i] && String[i] != '.'; i++ )
		{
			if('0' > String[i] || String[i] > '9') {
				return 0;
			}
			val = val*10 + String[i] - '0';
		}
		if(val > 255) {
			return 0;
		}
		Addr[j] = val;
		
		if(String[i] == '.')
			i ++;
	}
	if( j != 4 ) {
		return 0;
	}
	if(String[i] != '\0') {
		return 0;
	}
	return 1;
}

/**
 * \brief Read an IPv6 Address
 * \param String	IPv6 colon-hex representation
 * \param Addr	Output 128-bit representation of IP address
 * \return Boolean success
 */
static int Net_ParseIPv6Addr(const char *String, uint8_t *Addr)
{
	 int	i = 0;
	 int	j, k;
	 int	val, split = -1, end;
	uint16_t	hi[8], low[8];
	
	for( j = 0; String[i] && j < 8; j ++ )
	{
		if(String[i] == ':') {
			if(split != -1) {
				return 0;
			}
			split = j;
			i ++;
			continue;
		}
		
		val = 0;
		for( k = 0; String[i] && String[i] != ':'; i++, k++ )
		{
			val *= 16;
			if('0' <= String[i] && String[i] <= '9')
				val += String[i] - '0';
			else if('A' <= String[i] && String[i] <= 'F')
				val += String[i] - 'A' + 10;
			else if('a' <= String[i] && String[i] <= 'f')
				val += String[i] - 'a' + 10;
			else {
				return 0;
			}
		}
		
		if(val > 0xFFFF) {
			return 0;
		}
		
		if(split == -1)
			hi[j] = val;
		else
			low[j-split] = val;
		
		if( String[i] == ':' ) {
			i ++;
		}
	}
	end = j;
	
	// Create final address
	// - First section
	for( j = 0; j < split; j ++ )
	{
		Addr[j*2] = hi[j]>>8;
		Addr[j*2+1] = hi[j]&0xFF;
	}
	// - Zero region
	for( ; j < 8 - (end - split); j++ )
	{
		Addr[j*2] = 0;
		Addr[j*2+1] = 0;
	}
	// - Tail section
	k = 0;
	for( ; j < 8; j ++, k++)
	{
		Addr[j*2] = low[k]>>8;
		Addr[j*2+1] = low[k]&0xFF;
	}
	
	return 1;
}

/**
 * \brief Parse an address from a string
 * \param String	String containing an IPv4/IPv6 address
 * \param Addr	Buffer for the address (must be >= 16 bytes)
 * \return Address type
 * \retval 0	Unknown address type
 * \retval 4	IPv4 Address
 * \retval 6	IPv6 Address
 */
int Net_ParseAddress(const char *String, void *Addr)
{
	if( Net_ParseIPv4Addr(String, Addr) )
		return 4;
	
	if( Net_ParseIPv6Addr(String, Addr) )
		return 6;
	
	return 0;
}

int Net_OpenSocket(int AddrType, void *Addr, const char *Filename)
{
	 int	addrLen = IPStack_GetAddressSize(AddrType);
	 int	i;
	uint8_t	*addrBuffer = Addr;
	char	hexAddr[addrLen*2+1];
	
	for( i = 0; i < addrLen; i ++ )
		sprintf(hexAddr+i*2, "%02x", addrBuffer[i]);
	
	if(Filename)
	{
		 int	len = snprintf(NULL, 0, "/Devices/ip/routes/@%i:%s/%s", AddrType, hexAddr, Filename);
		char	path[len+1];
		snprintf(path, 100, "/Devices/ip/routes/@%i:%s/%s", AddrType, hexAddr, Filename);
		return VFS_Open(path, VFS_OPENFLAG_READ|VFS_OPENFLAG_WRITE);
	}
	else
	{
		 int	len = snprintf(NULL, 0, "/Devices/ip/routes/@%i:%s", AddrType, hexAddr);
		char	path[len+1];
		snprintf(path, 100, "/Devices/ip/routes/@%i:%s", AddrType, hexAddr);
		return VFS_Open(path, VFS_OPENFLAG_READ);
	}
}

int Net_OpenSocket_TCPC(int AddrType, void *Addr, int Port)
{
	int fd = Net_OpenSocket(AddrType, Addr, "tcpc");
	if( fd == -1 )	return -1;
	
	VFS_IOCtl(fd, 5, &Port);	// Remote Port
        VFS_IOCtl(fd, 6, Addr);	// Remote address
	VFS_IOCtl(fd, 7, NULL);	// connect
	return fd;
}

