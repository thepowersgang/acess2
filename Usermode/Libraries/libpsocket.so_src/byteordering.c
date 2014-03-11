/*
 * Acess2 POSIX Sockets Library
 * - By John Hodge (thePowersGang)
 *
 * byteordering.c
 * - hton/ntoh
 */
#include <arpa/inet.h>

// === CODE ===
static uint32_t Flip32(uint32_t val)
{
	return (((val >> 24) & 0xFF) << 0)
		| (((val >> 16) & 0xFF) << 8)
		| (((val >> 8) & 0xFF) << 16)
		| (((val >> 0) & 0xFF) << 24)
		;
}

static uint16_t Flip16(uint16_t val)
{
	return (val >> 8) | (val << 8);
}

uint32_t htonl(uint32_t hostlong)
{
	#if BIG_ENDIAN
	return hostlong;
	#else
	return Flip32(hostlong);
	#endif
}
uint16_t htons(uint16_t hostshort)
{
	#if BIG_ENDIAN
	return hostshort;
	#else
	return Flip16(hostshort);
	#endif
}
uint32_t ntohl(uint32_t netlong)
{
	#if BIG_ENDIAN
	return netlong;
	#else
	return Flip32(netlong);
	#endif
}
uint16_t ntohs(uint16_t netshort)
{
	#if BIG_ENDIAN
	return netshort;
	#else
	return Flip16(netshort);
	#endif
}


