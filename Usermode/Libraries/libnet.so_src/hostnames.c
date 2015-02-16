/*
 * Acess2 Networking Toolkit
 * By John Hodge (thePowersGang)
 * 
 * dns.c
 * - Hostname<->Address resolution
 */
#include <net.h>
#include "include/dns.h"
#include <string.h>
#include <stdlib.h>	// malloc (for loading config)
#include <stdbool.h>
#include <acess/sys.h>	// _SysDebug

// === TYPES ===
struct sDNSServer
{
	 int	AddrType;
	char	AddrData[16];
};

struct sHostEntry
{
	 int	AddrType;
	char	AddrData[16];
	char	*Names[];
};

struct sLookupAnyInfo
{
	 int	expected_type;
	void	*dest_ptr;
	bool	have_result;
};
struct sDNSCallbackInfo
{
	void	*cb_info;
	tNet_LookupAddrs_Callback	*callback;
	enum eTypes	desired_type;
	enum eClass	desired_class;
	bool	got_value;
};

// === PROTOTYPES ===
 int	int_lookupany_callback(void *info_v, int AddrType, const void *Addr);
void	int_DNS_callback(void *info, const char *name, enum eTypes type, enum eClass class, unsigned int ttl, size_t rdlength, const void *rdata);

// === GLOBALS ===
 int	giNumDNSServers;
struct sDNSServer	*gaDNSServers;
 int	giNumHostEntries;
struct sHostEntry	*gaHostEntries;

// === CODE ===
int Net_Lookup_AnyAddr(const char *Name, int AddrType, void *Addr)
{
	struct sLookupAnyInfo	cb_info = {
		.expected_type = AddrType,
		.dest_ptr = Addr,
		.have_result = false,
		};
	return Net_Lookup_Addrs(Name, &cb_info, int_lookupany_callback);
}
int int_lookupany_callback(void *info_v, int AddrType, const void *Addr)
{
	struct sLookupAnyInfo	*info = info_v;
	if( AddrType == info->expected_type && info->have_result == false )
	{
		memcpy(info->dest_ptr, Addr, Net_GetAddressSize(AddrType));
		
		info->have_result = true;
		return 1;
	}
	return 0;
}

int Net_Lookup_Addrs(const char *Name, void *cb_info, tNet_LookupAddrs_Callback *callback)
{
	_SysDebug("Net_Lookup_Addrs(Name='%s')", Name);
	// 1. Load (if not loaded) the DNS config from "/Acess/Conf/dns"
	// - "* <ip> <ip>" for DNS server(s)
	// - "127.0.0.1 localhost localhost.localdomain"
	if( !gaDNSServers )
	{
		giNumDNSServers = 1;
		gaDNSServers = malloc( 1 * sizeof(gaDNSServers[0]) );
		gaDNSServers[0].AddrType = Net_ParseAddress("192.168.1.1", gaDNSServers[0].AddrData);
	}
	
	// 2. Check the hosts list
	for( int i = 0; i < giNumHostEntries; i ++ )
	{
		const struct sHostEntry* he = &gaHostEntries[i];
		for( const char * const *namep = (const char**)he->Names; *namep; namep ++ )
		{
			if( strcasecmp(Name, *namep) == 0 )
			{
				if( callback(cb_info, he->AddrType, he->AddrData) != 0 )
					return 0;
			}
		}
	}
	// 3. Contact DNS server specified in config
	for( int i = 0; i < giNumDNSServers; i ++ )
	{
		const struct sDNSServer	*s = &gaDNSServers[i];
		struct sDNSCallbackInfo	info = {
			.cb_info = cb_info,
			.callback = callback,
			.desired_type = TYPE_A,
			.desired_class = CLASS_IN,
			.got_value = false,
			};
		if( ! DNS_Query(s->AddrType, s->AddrData, Name, info.desired_type, info.desired_class, int_DNS_callback, &info) )
		{
			if( info.got_value )
			{
				return 0;
			}
			else
			{
				// NXDomain, I guess
				return 1;
			}
		}
	}
	return 1;
}

void int_DNS_callback(void *info_v, const char *name, enum eTypes type, enum eClass class, unsigned int ttl, size_t rdlength, const void *rdata)
{
	struct sDNSCallbackInfo	*info = info_v;
	_SysDebug("int_DNS_callback(name='%s', type=%i, class=%i)", name, type, class);
	
	// Check type matches (if pattern was provided)
	if( info->desired_type != QTYPE_STAR && type != info->desired_type )
		return ;
	if( info->desired_class != QCLASS_STAR && class != info->desired_class )
		return ;
	
	switch( type )
	{
	case TYPE_A:
		if( rdlength != 4 )
			return ;
		info->callback( info->cb_info, 4, rdata );
		break;
	//case TYPE_AAAA:
	//	if( rdlength != 16 )
	//		return ;
	//	info->callback( info->cb_info, 6, rdata );
	//	break;
	default:
		// Ignore anything not A/AAAA
		break;
	}
	info->got_value = true;
}

int Net_Lookup_Name(int AddrType, const void *Addr, char *Dest[256])
{
	return 1;
}

