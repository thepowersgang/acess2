/*
 * Acess2 C Library
 *
 * sys/sockets.h
 * - POSIX Sockets
 *
 * By John Hodge (thePowersGang)
 */
#ifndef _SYS_SOCKETS_H_
#define _SYS_SOCKETS_H_

#include <sys/types.h>

typedef int	socklen_t;

typedef enum
{
	AF_UNSPEC	= 0,
	AF_PACKET	= 1,
	AF_INET 	= 4,
	AF_INET6	= 6,
} sa_family_t;

struct sockaddr
{
	sa_family_t	sa_family;
	char       	sa_data[16];
};

/**
 * \brief Values for \a domain of socket()
 */
enum eSocketDomains
{
	PF_LOCAL,	//!< Machine-local comms
	PF_INET,	//!< IPv4
	PF_INET6,	//!< IPv6
	PF_PACKET	//!< Low level packet interface
};
#define PF_UNIX	PF_LOCAL

enum eSocketTypes
{
	SOCK_STREAM,	//!< Stream (TCP)
	SOCK_DGRAM,	//!< Datagram (UDP)
	SOCK_SEQPACKET,	//!< 
	SOCK_RAW,	//!< Raw packet access
	SOCK_RDM	//!< Reliable non-ordered datagrams
};

/**
 * \brief Create a new socket descriptor
 * \param domain	Address family
 */
extern int	socket(int domain, int type, int protocol);

/**
 * \brief Bind a socket to an address
 */
extern int	bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

/**
 * \brief Connect to another socket
 */
extern int	connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

/**
 * \brief Listen for incoming connections
 */
extern int	listen(int sockfd, int backlog);

/**
 * \brief Accept an incoming connection
 */
extern int	accept(int sockfd, struct sockaddr *clientaddr, socklen_t addrlen);

extern int	recvfrom(int sockfd, void *buffer, size_t length, int flags, struct sockaddr *clientaddr, socklen_t *addrlen);
extern int	recv(int sockfd, void *buffer, size_t length, int flags);
extern int	sendto(int sockfd, const void *buffer, size_t length, int flags, const struct sockaddr *clientaddr, socklen_t addrlen);
extern int	send(int sockfd, void *buffer, size_t length, int flags);

#endif

