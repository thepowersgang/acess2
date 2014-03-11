/*
 * Acess2 POSIX Sockets Emulation
 * - By John Hodge (thePowersGang)
 *
 * arpa/inet.h
 * - 
 */
#ifndef _LIBPSOCKET__ARPA__INET_H_
#define _LIBPSOCKET__ARPA__INET_H_

#include <netinet/in.h>
#include <stdint.h>	// Should be inttypes.h?

#ifdef __cplusplus
extern "C" {
#endif

extern uint32_t htonl(uint32_t hostlong);
extern uint16_t htons(uint16_t hostshort);
extern uint32_t ntohl(uint32_t netlong);
extern uint16_t ntohs(uint16_t netshort);

extern in_addr_t	inet_addr(const char *cp);
extern in_addr_t	inet_lnaof(struct in_addr in);
extern struct in_addr	inet_makeaddr(in_addr_t net, in_addr_t lna);
extern in_addr_t	inet_netof(struct in_addr in);
extern in_addr_t	inet_network(const char *cp);
extern char	*inet_ntoa(struct in_addr in);

#ifdef __cplusplus
}
#endif

#endif

