/*
 * Acess2 Common Networking Library
 * By John Hodge (thePowersGang)
 */

#ifndef __LIBNET_H_
#define __LIBNET_H_

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
extern const char *Net_PrintAddress(int AddressType, void *Address);

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

#endif
