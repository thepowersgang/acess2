/*
 * Acess2 IP Stack
 *
 * hwaddr_resolution.c
 * - Resolution/caching of hardware addresses
 */
#define DEBUG	0
#include "ipstack.h"
#include "icmp.h"
#include "include/adapters_int.h"
#include "hwaddr_cache.h"
#include <timers.h>
#include <semaphore.h>
#include <limits.h>	// *INT_MAX

typedef struct sHWAdddrCache	tHWAddrCache;
typedef struct sHWAdddrCachedAddr	tHWAddrCachedAddr;

struct sHWAdddrCache {
	const tInterface	*Interface;
	tMutex	Lock;
	const int	MaxSize;
	 int	nAddrs;
	tHWAddrCachedAddr	*First;
};

struct sHWAdddrCachedAddr {
	tHWAddrCachedAddr	*Next;
	unsigned int	WaitingCount;	// While >0, cache will not be reused
	tSemaphore	WaitingSem;
	void	*L3Addr;
	tMacAddr	*HWAddr;
};

#define PRImacaddr	"%02x:%02x:%02x:%02x:%02x:%02x"
#define FMTmacaddr(a)	(a).B[0],(a).B[1],(a).B[2],(a).B[3],(a).B[4],(a).B[5]

// Cache is sorted by usage (most recently used at top)

// === CODE ===
tHWAddrCache *HWCache_int_GetCache(tAdapter *Adapter, int AddrType)
{
	static tHWAddrCache	cache_v6 = {.MaxSize=64};
	static tHWAddrCache	cache_v4 = {.MaxSize=64};
	switch(AddrType)
	{
	case 4:	return &cache_v4;
	case 6:	return &cache_v6;
	default:
		return NULL;
	}
}

bool memiszero(const void *mem, size_t length)
{
	const Uint8	*mem8 = mem;
	while( length -- )
	{
		if(*mem8 != 0)	return false;
		mem8 ++;
	}
	return true;
}

tMacAddr HWCache_Resolve(tInterface *Interface, const void *DestAddr)
{
	const size_t	addrsize = IPStack_GetAddressSize(Interface->Type);
	tHWAddrCache	*cache = HWCache_int_GetCache(Interface->Adapter, Interface->Type);

	LOG("DestAddr=%s", IPStack_PrintAddress(Interface->Type, DestAddr));	

	// Detect sending packets outside of the local network
	if( IPStack_CompareAddress(Interface->Type, DestAddr, Interface->Address, Interface->SubnetBits) == 0 )
	{
		// Off-net, use a route
		tRoute	*route = IPStack_FindRoute(Interface->Type, Interface, DestAddr);
		// If a route exists and the gateway isn't empty, use the gateway
		if( route )
		{
			if( !memiszero(route->NextHop, addrsize) )
			{
				// Update DestAddr to gateway address
				DestAddr = route->NextHop;
				LOG("Via gateway");
			}
			else
			{
				// Zero gateway means force direct
				LOG("Force direct");
			}
		}
		else
		{
			// No route to host
			LOG("No route to host");
			return cMAC_ZERO;
		}
	}
	// Local broadcast
	else if( IPStack_AddressIsBroadcast(Interface->Type, DestAddr, Interface->SubnetBits) )
	{
		// Broadcast, send to bcast mac
		LOG("Broadcast");
		return cMAC_BROADCAST;
	}
	else
	{
		// Fall through
	}
	LOG("DestAddr(2)=%s", IPStack_PrintAddress(Interface->Type, DestAddr));	
	
	Mutex_Acquire(&cache->Lock);
	// 1. Search cache for this address
	tHWAddrCachedAddr **pnp = &cache->First, *ca;
	tHWAddrCachedAddr *last = NULL;
	for( ca = cache->First; ca; ca = ca->Next )
	{
		LOG("ca = %p (%s => "PRImacaddr")",
			ca, IPStack_PrintAddress(Interface->Type, ca->L3Addr),
			FMTmacaddr(*ca->HWAddr)
			);
		if( memcmp(ca->L3Addr, DestAddr, addrsize) == 0 )
			break;
		last = ca;
		pnp = &ca->Next;
	}
	if( ca )
	{
		// Move to front, return
		if( cache->First != ca )
		{
			ASSERT(pnp != &cache->First);
			*pnp = ca->Next;

			ca->Next = cache->First;
			cache->First = ca;
			LOG("%p(%s) bumped", ca, IPStack_PrintAddress(Interface->Type, ca->L3Addr));
		}
		
		// If there's something waiting on this, odds are it's not populated
		if( ca->WaitingCount > 0 )
		{
			ASSERT(ca->WaitingCount != UINT_MAX);
			ca->WaitingCount ++;
			Mutex_Release(&cache->Lock);
			
			// Wait until populated
			LOG("Waiting on %p", ca);
			Semaphore_Wait(&ca->WaitingSem, 1);
			
			Mutex_Acquire(&cache->Lock);
			ASSERT(ca->WaitingCount > 0);
			ca->WaitingCount --;
		}
		tMacAddr ret = *(tMacAddr*)ca->HWAddr;
		Mutex_Release(&cache->Lock);
		
		LOG("Cached "PRImacaddr, FMTmacaddr(ret));
		return ret;
	}
	// 3. If not found:
	if( cache->nAddrs >= cache->MaxSize )
	{
		ASSERTC(cache->nAddrs, ==, cache->MaxSize);
		ASSERT(last);
		// TODO: Need to pick the oldest entry with WaitingThreads==0
		ASSERT(ca->WaitingCount == 0);
		// Reuse the oldest item
		ca = last;
		LOG("Reuse entry for %p(%s)", ca, IPStack_PrintAddress(Interface->Type, ca->L3Addr));
	}
	else
	{
		cache->nAddrs ++;
		ca = calloc( 1, sizeof(*ca) + addrsize + sizeof(tMacAddr) );
		ca->L3Addr = ca+1;
		ca->HWAddr = (void*)( (char*)ca->L3Addr + addrsize );
		LOG("New entry %p", ca);
	}
	memcpy(ca->L3Addr, DestAddr, addrsize);
	memset(ca->HWAddr, 0, sizeof(tMacAddr));

	// Shift to front of list
	if( cache->First != ca )
	{
		*pnp = ca->Next;
		ca->Next = cache->First;
		cache->First = ca;
	}
	
	// Mark cache entry as being waited upon
	ASSERT(ca->WaitingCount == 0);
	ca->WaitingCount = 1;
	// Then release cache lock (so inbound packets can manipulate it)
	Mutex_Release(&cache->Lock);

	// Send a request for the address
	switch(Interface->Type)
	{
	case 4:	ARP_Request4(Interface, *(tIPv4*)DestAddr);	break;
	//case 6:	ICMPv6_RequestND(Interface, DestAddr);	break;
	default:
		ASSERTC(Interface->Type, ==, 4);
		ASSERTC(Interface->Type, ==, 6);
		break;
	}

	// Wait for up to 3000ms for the entry to populate
	LOG("Waiting on new entry");
	Time_ScheduleEvent(3000);
	int rv = Semaphore_Wait(&ca->WaitingSem, 1);
	
	// Lock, reduce waiting count, grab return, and release
	Mutex_Acquire(&cache->Lock);
	ASSERT(ca->WaitingCount > 0);
	ca->WaitingCount --;
	tMacAddr	ret = *(tMacAddr*)ca->HWAddr;
	Mutex_Release(&cache->Lock);
	
	// NOTE: If entry wasn't populated, we'd return 0 anyway, this is
	//   just for logging purposes
	if( rv != 1 )
	{
		// Interrupted, return zero MAC
		LOG("Timeout/interrupt, return zero");
		return cMAC_ZERO;
	}

	// Release `ca` (on error, HWAddr will be nul)
	LOG("Requested "PRImacaddr, FMTmacaddr(ret));
	return ret;
}

void HWCache_Set(tAdapter *Adapter, int AddrType, const void *L3Addr, const tMacAddr *HWAddr)
{
	const size_t	addrsize = IPStack_GetAddressSize(AddrType);
	tHWAddrCache	*cache = HWCache_int_GetCache(Adapter, AddrType);
	LOG("Set %s = "PRImacaddr,
		IPStack_PrintAddress(AddrType, L3Addr),
		FMTmacaddr(*HWAddr)
		);
	// 1. Locate an existing entry
	Mutex_Acquire(&cache->Lock);
	tHWAddrCachedAddr	*last_unused = NULL;
	tHWAddrCachedAddr	**pnp = &cache->First;
	for( tHWAddrCachedAddr *ca = cache->First; ca; ca = ca->Next )
	{
		ASSERT(ca->Next != ca);
		ASSERT(!ca->Next || ca->Next->Next != ca);
		
		LOG("ca = %p (%s => "PRImacaddr")",
			ca, IPStack_PrintAddress(AddrType, ca->L3Addr), FMTmacaddr(*ca->HWAddr)
			);
		if( ca->WaitingCount == 0 )
			last_unused = ca;
		pnp = &ca->Next;
		
		// 2. If found, set the cache and poke waiting threads
		if( memcmp(L3Addr, ca->L3Addr, addrsize) == 0 )
		{
			if( ca->WaitingCount )
			{
				memcpy(ca->HWAddr, HWAddr, sizeof(tMacAddr));
				Semaphore_Signal(&ca->WaitingSem, INT_MAX);
				LOG("Found and cached");
			}
			else if( memcmp(HWAddr, ca->HWAddr, sizeof(tMacAddr)) )
			{
				LOG("Differs to cache");
			}
			else
			{
				LOG("Already known");
			}
			Mutex_Release(&cache->Lock);
			return ;
		}
	}
	// No existing entry, cache just in case
	if( cache->nAddrs < cache->MaxSize )
	{
		// Create new
		cache->nAddrs ++;
		tHWAddrCachedAddr *ca = calloc(1,sizeof(tHWAddrCachedAddr)+addrsize+sizeof(tMacAddr));
		*pnp = ca;
		ca->L3Addr = ca+1;
		ca->HWAddr = (void*)( (char*)ca->L3Addr + addrsize );
		memcpy(ca->L3Addr, L3Addr, addrsize);
		memcpy(ca->HWAddr, HWAddr, sizeof(tMacAddr));
		LOG("Cache in new entry");
	}
	else if( last_unused )
	{
		tHWAddrCachedAddr *ca = last_unused;
		memcpy(ca->L3Addr, L3Addr, addrsize);
		memcpy(ca->HWAddr, HWAddr, sizeof(tMacAddr));
		// Maintain position
		LOG("Cache in old entry");
	}
	else
	{
		// None unused! What is this madness?
		LOG("Not cached... cache being thrashed?");
	}
	Mutex_Release(&cache->Lock);
}
