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
#include <stdbool.h>

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

struct sDNSCallbackInfo
{
	 int	expected_size;
	void	*dest_ptr;
	enum eTypes	desired_type;
	enum eClass	desired_class;
	bool	have_result;
};

// === PROTOTYPES ===
void	int_DNS_callback(void *info, const char *name, enum eTypes type, enum eClass class, unsigned int ttl, size_t rdlength, const void *rdata);

// === GLOBALS ===
 int	giNumDNSServers;
struct sDNSServer	*gaDNSServers;
 int	giNumHostEntries;
struct sHostEntry	*gaHostEntries;

// === CODE ===
int Net_Lookup_AnyAddr(const char *Name, int AddrType, void *Addr)
{
	// 1. Load (if not loaded) the DNS config from "/Acess/Conf/dns"
	// - "* <ip> <ip>" for DNS server(s)
	// - "127.0.0.1 localhost localhost.localdomain"
	
	// 2. Check the hosts list
	for( int i = 0; i < giNumHostEntries; i ++ )
	{
		const struct sHostEntry* he = &gaHostEntries[i];
		if( he->AddrType == AddrType )
		{
			for( const char * const *namep = (const char**)he->Names; *namep; namep ++ )
			{
				if( strcasecmp(Name, *namep) == 0 )
				{
					memcpy(Addr, he->AddrData, Net_GetAddressSize(AddrType));
					return 0;
				}
			}
		}
	}
	// 3. Contact DNS server specified in config
	for( int i = 0; i < giNumDNSServers; i ++ )
	{
		// TODO
		const struct sDNSServer	*s = &gaDNSServers[i];
		struct sDNSCallbackInfo	info = {
			.expected_size = Net_GetAddressSize(AddrType),
			.dest_ptr = Addr,
			.desired_type = TYPE_A,
			.desired_class = CLASS_IN,
			.have_result = false
			};
		if( ! DNS_Query(s->AddrType, s->AddrData, Name, info.desired_type, info.desired_class, int_DNS_callback, &info) )
		{
			if( info.have_result )
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
	if( type == info->desired_type && class == info->desired_class && info->have_result == false )
	{
		// We're just working with A and AAAA, so copying from rdata is safe
		if( rdlength != info->expected_size ) {
			// ... oh, that's not good
			return ;
		}
		
		memcpy(info->dest_ptr, rdata, rdlength);
		
		info->have_result = true;
	}
}

int Net_Lookup_Name(int AddrType, const void *Addr, char *Dest[256])
{
	return 1;
}

