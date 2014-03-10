/*
 * Acess2 Common Networking Library
 * By John Hodge (thePowersGang)
 */

#ifndef __LIBNET_H_
#define __LIBNET_H_

enum {
	NET_ADDRTYPE_NULL = 0,
	NET_ADDRTYPE_IPV4 = 4,
	NET_ADDRTYPE_IPV6 = 6
};

/**
 * \brief Parse a string as an IP Address
 * \param String	Input string
 * \param Addr	Output binary format of the address
 * \return Address family (0: Invalid, 4: IPv4, 6: IPv6)
 */
extern int	Net_ParseAddress(const char *String, void *Addr);

/**
 * \brief Convert a network address into a string
 * \param AddressType	Address family as returned by Net_ParseAddress
 * \param Address	Address data
 */
extern const char *Net_PrintAddress(int AddressType, const void *Address);

/**
 * \brief Get the size in bytes of an address type
 * \param AddressType	Address type returned by Net_ParseAddress
 * \return Size of an address in bytes
 */
extern int Net_GetAddressSize(int AddressType);

/**
 * \brief Get the interface required to reach \a Addr
 * \param AddrType	Addresss Family (4: IPv4, 6: IPv6)
 * \param Addr	Address in binary format
 * \return Interface number
 */
extern char	*Net_GetInterface(int AddrType, void *Addr);

/**
 * \brief Open a network socket file
 * \param AddrType	Address family
 * \param Addr	Binary address
 * \param SocketName	Socket type to open (e.g. tcpc for TCP client)
 *                      If NULL, the node directory is opened
 * \return Socket file descriptor (as returned by \a open), or -1 on error
 * 
 * Opens a file using /Devices/ip/routes/@<AddrType>:<Addr>/<SocketName>
 * 
 */
extern int	Net_OpenSocket(int AddrType, void *Addr, const char *SocketName);

extern int	Net_OpenSocket_TCPC(int AddrType, void *Addr, int Port);

#endif
