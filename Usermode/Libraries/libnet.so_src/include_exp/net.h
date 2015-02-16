/*
 * Acess2 Common Networking Library
 * By John Hodge (thePowersGang)
 */

#ifndef __LIBNET_H_
#define __LIBNET_H_

#include <stddef.h>

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
extern int	Net_OpenSocket(int AddrType, const void *Addr, const char *SocketName);

extern int	Net_OpenSocket_TCPC(int AddrType, const void *Addr, int Port);

extern int	Net_OpenSocket_UDP(int AddrType, const void *Addr, int RAddr, int LAddr);
extern int Net_UDP_SendTo  (int FD, int Port, int AddrType, const void *Addr, size_t Length, const void *Data);
extern int Net_UDP_RecvFrom(int FD, int* Port, int* AddrType, void *Addr, size_t Length, void *Data);


/**
 * \name Hostnames
 * \brief Handling of hostname resolution
 * \{
 */

/**
 * \brief Returns an address for the specified hostname
 * \note Picks randomly if multiple addresses are present
 */
extern int	Net_Lookup_AnyAddr(const char *Name, int AddrType, void *Addr);

/**
 * \brief Callback for Net_Lookup_Addrs, returns non-zero when lookup should terminate
 */
typedef int tNet_LookupAddrs_Callback(void *info, int AddrType, const void *Addr);

/**
 * \brief Enumerate addresses for a host, calling the provided function for each
 */
extern int	Net_Lookup_Addrs(const char *Name, void *info, tNet_LookupAddrs_Callback* callback);

/**
 */
extern int	Net_Lookup_Name(int AddrType, const void *Addr, char *Dest[256]);

/**
 * \}
 */

#endif
