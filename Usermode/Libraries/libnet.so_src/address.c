/*
 * Acess2 Networking Toolkit
 * By John Hodge (thePowersGang)
 * 
 * address.c
 * - Address Parsing
 */
#include <net.h>
#include <stdint.h>
//#include <stdio.h>
#include <stdlib.h>
#define DEBUG	0

static inline uint32_t htonl(uint32_t v)
{
	return	  (((v >> 24) & 0xFF) <<  0)
		| (((v >> 16) & 0xFF) <<  8)
		| (((v >>  8) & 0xFF) << 16)
		| (((v >>  0) & 0xFF) << 24);
}
static inline uint16_t htons(uint16_t v)
{
	return	  (((v >> 8) & 0xFF) <<  0)
		| (((v >> 0) & 0xFF) <<  8);
}
#define htonb(v)	v
#define ntohl(v)	htonl(v)
#define ntohs(v)	htons(v)
#define ntohb(v)	v

#define __thread		// Disable TLS

/**
 * \brief Read an IPv4 Address
 * \param String	IPv4 dotted decimal address
 * \param Addr	Output 32-bit representation of IP address
 * \return Boolean success
 */
static int Net_ParseIPv4Addr(const char *String, uint8_t *Addr)
{
	 int	j;
	const char *pos = String;
	
	for( j = 0; *pos && j < 4; j ++ )
	{
		char	*end;
		unsigned long val = strtoul(pos, &end, 10);
		if( *end && *end != '.' ) {
			#if DEBUG
			_SysDebug("%s: Unexpected character, '%c' found", __func__, *end);
			#endif
			return 0;
		}
		if( *pos == '.' ) {
			#if DEBUG
			_SysDebug("%s: Two dots in a row", __func__);
			#endif
			return 0;
		}
		if(val > 255) {
			#if DEBUG
			_SysDebug("%s: val > 255 (%i)", __func__, val);
			#endif
			return 0;
		}
		#if DEBUG
		_SysDebug("%s: Comp '%.*s' = %lu", __func__, end - pos, pos, val);
		#endif
		Addr[j] = val;
		
		pos = end;
		
		if(*pos == '.')
			pos ++;
	}
	if( j != 4 ) {
		#if DEBUG
		_SysDebug("%s: 4 parts expected, %i found", __func__, j);
		#endif
		return 0;
	}
	if(*pos != '\0') {
		#if DEBUG
		_SysDebug("%s: EOS != '\\0', '%c'", __func__, *pos);
		#endif
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
				#if DEBUG
				printf("Two '::'s\n");
				#endif
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
				#if DEBUG
				printf("%c unexpected\n", String[i]);
				#endif
				return 0;
			}
		}
		
		if(val > 0xFFFF) {
			#if DEBUG
			printf("val (0x%x) > 0xFFFF\n", val);
			#endif
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

static const char *Net_PrintIPv4Address(const uint8_t *Address)
{
	static __thread char	ret[4*3+3+1];	// '255.255.255.255\0'
	
	sprintf(ret, "%i.%i.%i.%i", Address[0], Address[1], Address[2], Address[3]);
	
	return ret;
}

static const char *Net_PrintIPv6Address(const uint16_t *Address)
{
	static __thread char	ret[8*4+7+1];	// 'FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF:FFFF\0'
	#if 0
	// TODO: Zero compression
	 int	zeroStart = 0, zeroEnd = 8;
	for( i = 0; i < 8; i ++ ) {
		if( 
	}
	#endif
	
	sprintf(ret, "%x:%x:%x:%x:%x:%x:%x:%x",
		ntohs(Address[0]), ntohs(Address[1]), ntohs(Address[2]), ntohs(Address[3]),
		ntohs(Address[4]), ntohs(Address[5]), ntohs(Address[6]), ntohs(Address[7])
		);
	
	return ret;
}

const char *Net_PrintAddress(int AddressType, const void *Address)
{
	switch( AddressType )
	{
	case 0:	return "";
	
	case 4:
		return Net_PrintIPv4Address(Address);
	
	case 6:
		return Net_PrintIPv6Address(Address);
		
	default:
		return "BAD";
	}
}
