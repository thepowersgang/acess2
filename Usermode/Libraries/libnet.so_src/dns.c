/*
 * Acess2 Networking Toolkit
 * By John Hodge (thePowersGang)
 * 
 * dns.c
 * - Hostname<->Address resolution
 */
#include <stddef.h>	// size_t / NULL
#include <stdint.h>	// uint*_t
#include <string.h>	// memcpy, strchrnul
#include <assert.h>
#include <net.h>
#include "include/dns.h"

// === PROTOTYPES ===
size_t	DNS_EncodeName(void *buf, const char *dotted_name);
int DNS_DecodeName(char dotted_name[256], const void *buf, size_t space);
int DNS_int_ParseRR(const void *buf, size_t space, char* name_p, enum eTypes* type_p, enum eClass* class_p, uint32_t* ttl_p, size_t* rdlength_p);
static uint16_t	get16(const void *buf);
static size_t put16(void *buf, uint16_t val);


// === CODE ===
int DNS_Query(int ServerAType, const void *ServerAddr, const char *name, enum eTypes type, enum eClass class, handle_record_t* handle_record, void *info)
{
	int namelen = DNS_EncodeName(NULL, name);
	assert(namelen < 256);
	size_t	pos = 0;
	char	packet[ 512 ];
	assert( (6*2) + (namelen + 2*2) < 512 );
	// - Header
	pos += put16(packet + pos, 0xAC00);	// Identifier (arbitary)
	pos += put16(packet + pos, (0 << 0) | (0 << 1) );	// Op : Query, Standard, no other flags
	pos += put16(packet + pos, 1);	// QDCount
	pos += put16(packet + pos, 0);	// ANCount
	pos += put16(packet + pos, 0);	// NSCount
	pos += put16(packet + pos, 0);	// ARCount
	// - Question
	pos += DNS_EncodeName(packet + pos, name);
	pos += put16(packet + pos, type);	// QType
	pos += put16(packet + pos, class);	// QClass
	
	assert(pos <= sizeof(packet));
	
	// Send and wait for reply
	// - Lock
	//  > TODO: Lock DNS queries
	// - Send
	int sock = Net_OpenSocket_UDP(ServerAType, ServerAddr, 53, 0);
	if( sock < 0 ) {
		// Connection failed
		// TODO: Correctly report this failure with a useful error code
		return 1;
	}
	int rv = _SysWrite(sock, packet, pos);
	if( rv != pos ) {
		// TODO: Error reporting
		_SysClose(sock);
		return 1;
	}
	// - Wait
	int return_len = 0;
	do {
		return_len = _SysRead(sock, packet, sizeof(packet));
	} while( return_len == 0 );
	if( return_len < 0 ) {
		// TODO: Error reporting
		_SysClose(sock);
		return 1;
	}
	_SysClose(sock);
	// - Release
	//  > TODO: Lock DNS queries
	
	// For each response in the answer (and additional) sections, call the passed callback
	char	rr_name[256];
	unsigned int qd_count = get16(packet + 4);
	unsigned int an_count = get16(packet + 6);
	unsigned int ns_count = get16(packet + 8);
	unsigned int ar_count = get16(packet + 10);
	pos = 6*2;
	// TODO: Can I safely assert / fail if qd_count is non-zero?
	// - Questions, ignored
	for( unsigned int i = 0; i < qd_count; i ++ ) {
		pos += DNS_DecodeName(NULL, packet + pos, return_len - pos);
		pos += 2*2;
	}
	// - Answers, pass on to handler
	for( unsigned int i = 0; i < an_count; i ++ )
	{
		enum eTypes	type;
		enum eClass	class;
		uint32_t	ttl;
		size_t	rdlength;
		int rv = DNS_int_ParseRR(packet + pos, return_len - pos, rr_name, &type, &class, &ttl, &rdlength);
		if( rv < 0 ) {
			return 1;
		}
		pos += rv;
		
		handle_record(info, rr_name, type, class, ttl, rdlength, packet + pos);
	}
	// Authority Records (should all be NS records)
	for( unsigned int i = 0; i < ns_count; i ++ )
	{
		size_t	rdlength;
		int rv = DNS_int_ParseRR(packet + pos, return_len - pos, rr_name, NULL, NULL, NULL, &rdlength);
		if( rv < 0 ) {
			return 1;
		}
		pos += rv;
	}
	// - Additional records, pass to handler
	for( unsigned int i = 0; i < ar_count; i ++ )
	{
		enum eTypes	type;
		enum eClass	class;
		uint32_t	ttl;
		size_t	rdlength;
		int rv = DNS_int_ParseRR(packet + pos, return_len - pos, rr_name, &type, &class, &ttl, &rdlength);
		if( rv < 0 ) {
			return 1;
		}
		pos += rv;
		
		handle_record(info, rr_name, type, class, ttl, rdlength, packet + pos);
	}

	return 0;
}

/// Encode a dotted name as a DNS name
size_t	DNS_EncodeName(void *buf, const char *dotted_name)
{
	size_t	ret = 0;
	const char *str = dotted_name;
	uint8_t	*buf8 = buf;
	while( *str )
	{
		const char *next = strchr(str, '.');
		size_t seg_len = (next ? next - str : strlen(str));
		if( seg_len > 63 ) {
			// Oops, too long (truncate)
			seg_len = 63;
		}
		if( seg_len == 0 && next != NULL ) {
			// '..' encountered, invalid (skip)
			str = next+1;
			continue ;
		}
		
		if( buf8 )
		{
			buf8[ret] = seg_len;
			memcpy(buf8+ret+1, str, seg_len);
		}
		ret += 1 + seg_len;
		
		if( next == NULL ) {
			// No trailing '.', assume it's there? Yes, need to be NUL terminated
			if(buf8)	buf8[ret] = 0;
			ret ++;
			break;
		}
		else {
			str = next + 1;
		}
	}
	return ret;
}

// Decode a name (including trailing . for root)
int DNS_DecodeName(char dotted_name[256], const void *buf, size_t space)
{
	int consumed = 0;
	int out_pos = 0;
	const uint8_t *buf8 = buf;
	while( *buf8 && space > 0 )
	{
		if( consumed + 1 > space )	return -1;
		uint8_t	seg_len = *buf8;
		buf8 ++;
		consumed ++;
		// Protocol violation (overflowed end of buffer)
		if( consumed + seg_len > space )
			return -1;
		// Protocol violation (segment too long)
		if( seg_len >= 64 )
			return -1;
		// Protocol violation (name was too long)
		if( out_pos + seg_len + 1 > sizeof(dotted_name)-1 )
			return -1;
		
		// Read segment
		memcpy(dotted_name + out_pos, buf8, seg_len);
		buf8 += seg_len;
		consumed += seg_len;
		
		// Place '.'
		dotted_name[out_pos+seg_len+1] = '.';
		// Increment output counter
		out_pos += seg_len + 1;
	}
	
	dotted_name[out_pos] = '\0';
	return consumed;
}

// Parse a Resource Record
int DNS_int_ParseRR(const void *buf, size_t space, char* name_p, enum eTypes* type_p, enum eClass* class_p, uint32_t* ttl_p, size_t* rdlength_p)
{
	const uint8_t	*buf8 = buf;
	size_t	consumed = 0;
	
	// 1. Name
	int rv = DNS_DecodeName(name_p, buf8, space);
	if(rv < 0)	return -1;
	
	buf8 += rv, consumed += rv;
	
	if( type_p )
		*type_p = get16(buf8);
	buf8 += 2, consumed += 2;
	
	if( class_p )
		*class_p = get16(buf8);
	buf8 += 2, consumed += 2;
	
	return consumed;
}

static uint16_t get16(const void *buf) {
	const uint8_t* buf8 = buf;
	uint16_t rv = 0;
	rv |= buf8[0];
	rv |= (uint16_t)buf8[1] << 8;
	return rv;
}
static size_t put16(void *buf, uint16_t val) {
	uint8_t* buf8 = buf;
	buf8[0] = val & 0xFF;
	buf8[1] = val >> 8;
	return 2;
}

