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
#include "include/dns_int.h"

// === PROTOTYPES ===
//int DNS_Query(int ServerAType, const void *ServerAddr, const char *name, enum eTypes type, enum eClass class, handle_record_t* handle_record, void *info);

// === CODE ===
int DNS_Query(int ServerAType, const void *ServerAddr, const char *name, enum eTypes type, enum eClass class, handle_record_t* handle_record, void *info)
{
	char	packet[512];
	size_t packlen = DNS_int_EncodeQuery(packet, sizeof(packet), name, type, class);
	if( packlen == 0 ) {
		_SysDebug("DNS_Query - Serialising packet failed");
		return 2;
	}
	
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
	int rv = Net_UDP_SendTo(sock, 53, ServerAType, ServerAddr, packlen, packet);
	if( rv != packlen ) {
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

