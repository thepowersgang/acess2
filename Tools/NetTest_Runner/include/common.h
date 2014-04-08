/*
 */
#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>

#define HOST_IP_STR	"10.1.1.2"
#define HOST_IP 	0x0A,0x01,0x01,0x02
#define HOST_MAC	0x54,0x56,0x00,0x12,0x34,0x11

#define TEST_IP 	0x0A,0x01,0x01,0x01
#define TEST_MAC	0x54,0x56,0x00,0x12,0x34,0x56

#define BLOB(bytes)	(const char[]){bytes}

#if BIG_ENDIAN
# define htons(x)	((uint16_t)(x))
# define htonl(x)	((uint32_t)(x))
#else
static inline uint16_t	htons(uint16_t x) {
	return (x >> 8) | (x << 8);
}
static inline uint32_t	htonl(uint32_t x) {
	return ((x >> 24) & 0xFF) <<  0
	      |((x >> 16) & 0xFF) <<  8
	      |((x >>  8) & 0xFF) << 16
	      |((x >>  0) & 0xFF) << 24;
}
#endif
#define ntohs(x)	htons(x)
#define ntohl(x)	htonl(x)


#endif

