/*
 * Acess2 POSIX Sockets Emulation
 * - By John Hodge (thePowersGang)
 *
 * sys/sockets.h
 * - POSIX Sockets
 */
#ifndef _SYS_SOCKETS_H_
#define _SYS_SOCKETS_H_

#include <sys/types.h>
#include <stddef.h>	// size_t

typedef uint32_t	socklen_t;

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

struct msghdr
{
	void	*msg_name;
	socklen_t	msg_namelen;
	struct iovec	*msg_iov;
	int	msg_iovlen;
	void	*msg_control;
	socklen_t	msg_controllen;
	int	msg_flags;
};

struct cmsghdr
{
	socklen_t	cmsg_len;
	int	cmsg_level;
	int	cmsg_type;
};

#define SCM_RIGHTS	0x1

#define CMSG_DATA(cmsg)	((unsigned char*)(cmsg + 1))
#define CMSG_NXTHDR(mhdr, cmsg)	0
#define CMSG_FIRSTHDR(mhdr)	0

struct linger
{
	int	l_onoff;
	int	l_linger;
};

enum eSocketTypes
{
	SOCK_STREAM,	//!< Stream (TCP)
	SOCK_DGRAM,	//!< Datagram (UDP)
	SOCK_SEQPACKET,	//!< 
	SOCK_RAW,	//!< Raw packet access
	SOCK_RDM	//!< Reliable non-ordered datagrams
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

extern int	setsockopt(int socket, int level, int option_name, const void *option_value, socklen_t option_len);
extern int	getsockopt(int socket, int level, int option_name, void *option_value, socklen_t *option_len);

#endif

