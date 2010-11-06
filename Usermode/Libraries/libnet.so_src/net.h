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
 * \brief Get the interface required to reach \a Addr
 * \param AddrType	Addresss Family (4: IPv4, 6: IPv6)
 * \param Addr	Address in binary format
 * \return Interface number
 */
extern int	Net_GetInterface(int AddrType, void *Addr);

#endif
