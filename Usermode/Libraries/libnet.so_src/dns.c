/*
 * Acess2 Networking Toolkit
 * By John Hodge (thePowersGang)
 * 
 * dns.c
 * - Hostname<->Address resolution
 */
#include <stddef.h>	// size_t / NULL
#include <stdint.h>	// uint*_t
#include <string.h>	// memcpy, strchr
#include <assert.h>
#include <acess/sys.h>	// for _SysSelect
#include <acess/fd_set.h>	// FD_SET
#include <net.h>
#include "include/dns.h"

// === PROTOTYPES ===
//int DNS_Query(int ServerAType, const void *ServerAddr, const char *name, enum eTypes type, enum eClass class, handle_record_t* handle_record, void *info);
int DNS_int_ParseResponse(const void* packet, size_t return_len, void *info, handle_record_t* handle_record_t);
size_t	DNS_EncodeName(void *buf, const char *dotted_name);
int DNS_DecodeName(char dotted_name[256], const void *buf, size_t ofs, size_t space);
int DNS_int_ParseRR(const void *buf, size_t ofs, size_t space, char* name_p, enum eTypes* type_p, enum eClass* class_p, uint32_t* ttl_p, size_t* rdlength_p);

static uint16_t	get16(const void *buf);
static uint32_t	get32(const void *buf);
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
	pos += put16(packet + pos, (0 << 0) | (0 << 1) | (1 << 8) );	// Op : Query, Standard, Recursion
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
		_SysDebug("DNS_Query - UDP open failed");
		// TODO: Correctly report this failure with a useful error code
		return 1;
	}
	int rv = Net_UDP_SendTo(sock, 53, ServerAType, ServerAddr, pos, packet);
	if( rv != pos ) {
		_SysDebug("DNS_Query - Write failed");
		// TODO: Error reporting
		_SysClose(sock);
		return 1;
	}
	// - Wait
	{
		 int	nfd = sock + 1;
		fd_set	fds;
		FD_ZERO(&fds);
		FD_SET(sock, &fds);
		int64_t	timeout = 2000;	// Give it two seconds, should be long enough
		rv = _SysSelect(nfd, &fds, NULL, NULL, &timeout, 0);
		if( rv == 0 ) {
			// Timeout with no reply, give up
			_SysDebug("DNS_Query - Timeout");
			_SysClose(sock);
			return 1;
		}
		if( rv < 0 ) {
			// Oops, select failed
			_SysDebug("DNS_Query - Select failure");
			_SysClose(sock);
			return 1;
		}
	}
	int return_len = Net_UDP_RecvFrom(sock, NULL, NULL, NULL, sizeof(packet), packet);
	if( return_len <= 0 ) {
		// TODO: Error reporting
		_SysDebug("DNS_Query - Read failure");
		_SysClose(sock);
		return 1;
	}
	_SysClose(sock);
	// - Release
	//  > TODO: Lock DNS queries
	
	// For each response in the answer (and additional) sections, call the passed callback
	return DNS_int_ParseResponse(packet, return_len, info, handle_record);
}

int DNS_int_ParseResponse(const void* buf, size_t return_len, void *info, handle_record_t* handle_record)
{
	const uint8_t* packet = buf;
	char	rr_name[256];
	unsigned int id = get16(packet + 0);
	if( id != 0xAC00 ) {
		_SysDebug("DNS_Query - Packet ID mismatch");
		return 2;
	}
	unsigned int flags = get16(packet + 2);
	unsigned int qd_count = get16(packet + 4);
	unsigned int an_count = get16(packet + 6);
	unsigned int ns_count = get16(packet + 8);
	unsigned int ar_count = get16(packet + 10);
	size_t pos = 6*2;
	// TODO: Can I safely assert / fail if qd_count is non-zero?
	// - Questions, ignored
	for( unsigned int i = 0; i < qd_count; i ++ ) {
		int rv = DNS_DecodeName(rr_name, packet, pos, return_len);
		if( rv < 0 ) {
			_SysDebug("DNS_Query - Parse error in QD");
			return 1;
		}
		pos += rv + 2*2;
	}
	// - Answers, pass on to handler
	for( unsigned int i = 0; i < an_count; i ++ )
	{
		enum eTypes	type;
		enum eClass	class;
		uint32_t	ttl;
		size_t	rdlength;
		int rv = DNS_int_ParseRR(packet, pos, return_len, rr_name, &type, &class, &ttl, &rdlength);
		if( rv < 0 ) {
			_SysDebug("DNS_Query - Parse error in AN");
			return 1;
		}
		pos += rv;
		
		handle_record(info, rr_name, type, class, ttl, rdlength, packet + pos - rdlength);
	}
	// Authority Records (should all be NS records)
	for( unsigned int i = 0; i < ns_count; i ++ )
	{
		size_t	rdlength;
		int rv = DNS_int_ParseRR(packet, pos, return_len, rr_name, NULL, NULL, NULL, &rdlength);
		if( rv < 0 ) {
			_SysDebug("DNS_Query - Parse error in NS");
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
		int rv = DNS_int_ParseRR(packet, pos, return_len, rr_name, &type, &class, &ttl, &rdlength);
		if( rv < 0 ) {
			_SysDebug("DNS_Query - Parse error in AR");
			return 1;
		}
		pos += rv;
		
		handle_record(info, rr_name, type, class, ttl, rdlength, packet + pos - rdlength);
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
int DNS_DecodeName(char dotted_name[256], const void *buf, size_t ofs, size_t space)
{
	int consumed = 0;
	int out_pos = 0;
	const uint8_t *buf8 = (const uint8_t*)buf + ofs;
	for( ;; )
	{
		if( ofs + consumed + 1 > space ) {
			_SysDebug("DNS_DecodeName - Len byte OOR space=%i", space);
			return -1;
		}
		uint8_t	seg_len = *buf8;
		buf8 ++;
		consumed ++;
		// Done
		if( seg_len == 0 )
			break;
		if( (seg_len & 0xC0) == 0xC0 )
		{
			// Backreference, the rest of the name is a backref
			char tmp[256];
			int ref_ofs = get16(buf8 - 1) & 0x3FFF;
			consumed += 1, buf8 += 1;	// Only one, previous inc still applies
			_SysDebug("DNS_DecodeName - Nested at %i", ref_ofs);
			if( DNS_DecodeName(tmp, buf, ref_ofs, space) < 0 )
				return -1;
			memcpy(dotted_name+out_pos, tmp, strlen(tmp));
			out_pos += strlen(tmp);
			break;
		}
		// Protocol violation (segment too long)
		if( seg_len >= 64 ) {
			_SysDebug("DNS_DecodeName - Seg too long %i", seg_len);
			return -1;
		}
		// Protocol violation (overflowed end of buffer)
		if( ofs + consumed + seg_len > space ) {
			_SysDebug("DNS_DecodeName - Seg OOR %i+%i>%i", consumed, seg_len, space);
			return -1;
		}
		// Protocol violation (name was too long)
		if( out_pos + seg_len + 1 > 255 ) {
			_SysDebug("DNS_DecodeName - Dotted name too long %i+%i+1 > %i",
				out_pos, seg_len, 255);
			return -1;
		}
		
		_SysDebug("DNS_DecodeName : Seg %i '%.*s'", seg_len, seg_len, buf8);
		
		// Read segment
		memcpy(dotted_name + out_pos, buf8, seg_len);
		buf8 += seg_len;
		consumed += seg_len;
		out_pos += seg_len;
		
		// Place '.'
		dotted_name[out_pos] = '.';
		out_pos ++;
	}
	dotted_name[out_pos] = '\0';
	_SysDebug("DNS_DecodeName - '%s', consumed = %i", dotted_name, consumed);
	return consumed;
}

// Parse a Resource Record
int DNS_int_ParseRR(const void *buf, size_t ofs, size_t space, char* name_p, enum eTypes* type_p, enum eClass* class_p, uint32_t* ttl_p, size_t* rdlength_p)
{
	const uint8_t	*buf8 = buf;
	size_t	consumed = 0;
	
	// 1. Name
	int rv = DNS_DecodeName(name_p, buf, ofs, space);
	if(rv < 0)	return -1;
	
	ofs += rv, consumed += rv;
	
	if( type_p )
		*type_p = get16(buf8 + ofs);
	ofs += 2, consumed += 2;
	
	if( class_p )
		*class_p = get16(buf8 + ofs);
	ofs += 2, consumed += 2;
	
	if( ttl_p )
		*ttl_p = get32(buf + ofs);
	ofs += 4, consumed += 4;
	
	size_t rdlength = get16(buf + ofs);
	if( rdlength_p )
		*rdlength_p = rdlength;
	ofs += 2, consumed += 2;
	
	_SysDebug("DNS_int_ParseRR - name='%s', rdlength=%i", name_p, rdlength);
	
	return consumed + rdlength;
}

static uint16_t get16(const void *buf) {
	const uint8_t* buf8 = buf;
	uint16_t rv = 0;
	rv |= (uint16_t)buf8[0] << 8;
	rv |= (uint16_t)buf8[1] << 0;
	return rv;
}
static uint32_t get32(const void *buf) {
	const uint8_t* buf8 = buf;
	uint32_t rv = 0;
	rv |= (uint32_t)buf8[0] << 24;
	rv |= (uint32_t)buf8[1] << 16;
	rv |= (uint32_t)buf8[2] << 8;
	rv |= (uint32_t)buf8[3] << 0;
	return rv;
}
static size_t put16(void *buf, uint16_t val) {
	uint8_t* buf8 = buf;
	buf8[0] = val >> 8;
	buf8[1] = val & 0xFF;
	return 2;
}

