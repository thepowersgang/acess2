/*
 * Acess2 Networking Stack
 * - By John Hodge (thePowersGang)
 *
 * adapters.c
 * - Network Adapter Management
 */
#define DEBUG	0
#define VERSION	VER2(0,1)
#include "ipstack.h"
#include "include/buffer.h"
#include "include/adapters.h"
#include "include/adapters_int.h"
#include "link.h"
#include <api_drv_common.h>	// For the VFS hack
#include <api_drv_network.h>

// === PROTOTYPES ===
// --- "External" (NIC) API ---
void	*IPStack_Adapter_Add(const tIPStack_AdapterType *Type, void *Ptr, const void *HWAddr);
void	IPStack_Adapter_Del(void *Handle);
// --- VFS API ---
char	*Adapter_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*Adapter_FindDir(tVFS_Node *Node, const char *Name);
 int	Adapter_DirIOCtl(tVFS_Node *Node, int Num, void *Data);
 int	Adapter_IOCtl(tVFS_Node *Node, int Num, void *Data);
// --- "Internal" (IPStack) API ---
tAdapter	*Adapter_GetByName(const char *Name);
void	Adapter_SendPacket(tAdapter *Handle, tIPStackBuffer *Buffer);
// --- Helpers ---
void	Adapter_int_WatchThread(void *Ptr);
tIPStackBuffer	*Adapter_int_LoopbackWaitPacket(void *Unused);
 int	Adapter_int_LoopbackSendPacket(void *Unused, tIPStackBuffer *Buffer);

// === GLOBALS ===
// --- VFS Magic ---
tVFS_NodeType	gIP_AdaptersDirType = {
	.ReadDir = Adapter_ReadDir,
	.FindDir = Adapter_FindDir,
	.IOCtl = Adapter_DirIOCtl
};
tVFS_NodeType	gIP_AdapterType = {
	// TODO: Should direct IO be allowed?
	.IOCtl = Adapter_IOCtl
};
tVFS_Node	gIP_AdaptersNode = {
	.Type = &gIP_AdaptersDirType,
	.Flags = VFS_FFLAG_DIRECTORY,
	.Size = -1
};
// --- Loopback Adapter ---
tIPStack_AdapterType	gIP_LoopAdapterType = {
	.Name = "Loopback",
	.WaitForPacket = Adapter_int_LoopbackWaitPacket,
	.SendPacket = Adapter_int_LoopbackSendPacket
	};
tAdapter	gIP_LoopAdapter;
// --- Main adapter list ---
tMutex	glIP_Adapters;
tAdapter	*gpIP_AdapterList;
tAdapter	*gpIP_AdapterList_Last = (void*)&gpIP_AdapterList;	// HACK!
 int	giIP_NextAdapterIndex;

// === CODE ===
// --- "External" (NIC) API ---
void *IPStack_Adapter_Add(const tIPStack_AdapterType *Type, void *Ptr, const void *HWAddr)
{
	tAdapter	*ret;
	
	ret = malloc(sizeof(tAdapter) + 6);	// TODO: Don't assume 6-byte MAC addresses
	ret->Next = NULL;
	ret->Type = Type;
	ret->CardHandle = Ptr;
	ret->RefCount = 0;
	ret->Index = giIP_NextAdapterIndex++;
	memcpy(ret->HWAddr, HWAddr, 6);

	memset(&ret->Node, 0, sizeof(ret->Node));
	ret->Node.Type = &gIP_AdapterType;
	ret->Node.ImplPtr = ret;

	Mutex_Acquire( &glIP_Adapters );
	gpIP_AdapterList_Last->Next = ret;
	gpIP_AdapterList_Last = ret;
	Mutex_Release( &glIP_Adapters );
	
	// Watch the adapter for incoming packets
	tTID tid = Proc_SpawnWorker(Adapter_int_WatchThread, ret);
	if(tid < 0) {
		Log_Warning("IPStack", "Unable to create watcher thread for %p", ret);
	}
	
	return ret;
}

void *IPStack_Adapter_AddVFS(const char *Path)
{
	 int	fd, tmp;
	char	mac[6];

	ENTER("sPath", Path);	

	// Open Device
	fd = VFS_Open( Path, VFS_OPENFLAG_READ|VFS_OPENFLAG_WRITE );
	if( fd == -1 ) {
		LEAVE('n');
		return NULL;
	}
	
	// Check that it is a network interface
	tmp = VFS_IOCtl(fd, 0, NULL);
	LOG("Device type = %i", tmp);
	if( tmp != DRV_TYPE_NETWORK ) {
		Log_Warning("IPStack", "IPStack_Adapter_AddVFS: '%s' is not a network interface", Path);
		VFS_Close( fd );
		LEAVE('n');
		return NULL;
	}
	
	// Get MAC Address
	VFS_IOCtl(fd, NET_IOCTL_GETMAC, mac);

	return IPStack_Adapter_Add(NULL, (void*)(tVAddr)fd, mac);
}

void IPStack_Adapter_Del(void *Handle)
{
	Log_Error("IPStack", "TODO: Implement IPStack_Adapter_Del");
	// TODO: Allow removing adapters during system operation
}

// --- VFS API ---
char *Adapter_ReadDir(tVFS_Node *Node, int Pos)
{
	if( Pos < 0 )	return NULL;

	// Loopback
	if( Pos == 0 ) {
		return strdup("lo");
	}
	Pos --;
	
	#define CHECK_LIST(list, type) do{ \
		tAdapter *a;  int i;\
		for(i=0,a=list; i < Pos && a; i ++, a = a->Next ); \
		if( a ) { \
			return Adapter_GetName(a);\
		} \
		Pos -= i; \
	} while(0);

	CHECK_LIST(gpIP_AdapterList, "eth");
	// TODO: Support other types of adapters (wifi, tap, ...)

	return NULL;
}

tVFS_Node *Adapter_FindDir(tVFS_Node *Node, const char *Name)
{
	tAdapter *a = Adapter_GetByName(Name);
	if(!a)
		return NULL;
	else
		return &a->Node;
}

static const char *casIOCtls_Root[] = { DRV_IOCTLNAMES, "add_vfs_adapter", NULL };
int Adapter_DirIOCtl(tVFS_Node *Node, int Num, void *Data)
{
	switch(Num)
	{
	BASE_IOCTLS(DRV_TYPE_MISC, "IPStack-AdapterList", VERSION, casIOCtls_Root)
	
	case 4:
		if( !CheckString(Data) )
			return -1;

		// Return non-zero if add fails
		return IPStack_Adapter_AddVFS(Data) == NULL;
	}
	return -1;
}

static const char *casIOCtls_Dev[] = { DRV_IOCTLNAMES, "get_hwaddr", NULL };
int Adapter_IOCtl(tVFS_Node *Node, int Num, void *Data)
{
	tAdapter *adapter = Node->ImplPtr;
	switch(Num)
	{
	BASE_IOCTLS(DRV_TYPE_MISC, "IPStack-Adapter", VERSION, casIOCtls_Dev)
	
	case 4:
		if( !CheckMem(Data, 6) )
			return -1;

		memcpy(Data, adapter->HWAddr, 6);
		return 0;
	}
	return -1;
}

// --- "Internal" (IPStack) API ---
tAdapter *Adapter_GetByName(const char *Name)
{
	if( strcmp(Name, "lo") == 0 ) {
		return &gIP_LoopAdapter;
	}
	
	if( strncmp(Name, "eth", 3) == 0 )
	{
		// Possibly an ethX interface
		 int	index, ofs;
		
		// Get the index at the end
		ofs = ParseInt(Name + 3, &index);
		if( ofs == 0 || Name[3+ofs] != '\0' )
			return NULL;
		
		// Find the adapter with that index
		for( tAdapter *a = gpIP_AdapterList; a && a->Index <= index; a = a->Next ) {
			if( a->Index == index )
				return a;
		}
		
		return NULL;
	}
	
	return NULL;
}

char *Adapter_GetName(tAdapter *Adapter)
{
	if( Adapter == &gIP_LoopAdapter )
	{
		return strdup("lo");
	}
	else
	{
		// TODO: Support multiple adapter types
		char	buf[sizeof("eth")+10];
		sprintf(buf, "eth%i", Adapter->Index);
		return strdup(buf);
	}
}

void Adapter_SendPacket(tAdapter *Handle, tIPStackBuffer *Buffer)
{
	if( Handle->Type == NULL )
	{
		size_t outlen = 0;
		void *data = IPStack_Buffer_CompactBuffer(Buffer, &outlen);
		VFS_Write((tVAddr)Handle->CardHandle, outlen, data);
		free(data);
	}
	else
	{
		Handle->Type->SendPacket( Handle->CardHandle, Buffer );
	}
}

// --- Helpers ---
void Adapter_int_WatchThread(void *Ptr)
{
	tAdapter	*Adapter = Ptr;
	const int	MTU = 1520;
	tIPStackBuffer	*buf = NULL;
	void	*data = NULL;
	
	if( Adapter->Type == NULL )
	{
		data = malloc( MTU );
		buf = IPStack_Buffer_CreateBuffer(1);
	}

	
	Threads_SetName("Adapter Watcher");
	Log_Log("IPStack", "Thread %i watching eth%i '%s'", Threads_GetTID(), Adapter->Index,
		Adapter->Type?Adapter->Type->Name:"VFS");

	for( ;; )
	{
		if( Adapter->Type == NULL )
		{
			int len = VFS_Read((tVAddr)Adapter->CardHandle, MTU, data);
			IPStack_Buffer_AppendSubBuffer(buf, len, 0, buf, NULL, NULL);
		}
		else
		{
			buf = Adapter->Type->WaitForPacket( Adapter->CardHandle );
		}
	
		Link_HandlePacket(Adapter, buf);
	
		if( Adapter->Type == NULL )
		{
			IPStack_Buffer_ClearBuffer(buf);
		}
		else
		{
			IPStack_Buffer_DestroyBuffer(buf);
		}
	}
}

tIPStackBuffer *Adapter_int_LoopbackWaitPacket(void *Unused)
{
	Threads_Sleep();
	return NULL;
}

int Adapter_int_LoopbackSendPacket(void *Unused, tIPStackBuffer *Buffer)
{
	// This is a little hacky :)
	Link_HandlePacket(&gIP_LoopAdapter, Buffer);
	return 0;
}

// --- Broadcast Debugging ---
extern Uint16	IPv4_Checksum(const void *Buf, size_t Length);
void IPStack_SendDebugText(const char *Text)
{
	const Uint8	pkt_hdr[] = {
		0xFF,0xFF, 0xFF,0xFF, 0xFF,0xFF,
		0x00,0x00, 0x00,0x00, 0x00,0x00,
		0x08,0x00,
		
		0x45,0x00,	// Version/Length, DiffServices
		0xFF,0xFF,	// Total Length
		0x00,0x00,	// Identifcation
		0x00,0x00, 0xFF,0x11,	// Flags,Fragment, TTL=255,proto=UDP
		0x00,0x00,	// Header checksum
		0x00,0x00,0x00,0x00,	// Source
		0xFF,0xFF,0xFF,0xFF,	// Destination
		
		0x80,0x00, 0x80,0x00,
		0xFF,0xFF, 0xFF,0xFF,
	};
	static tShortSpinlock	lLock;

	// Fast return if there's no avaliable adapters
	if( !gpIP_AdapterList )
		return ;

	if( CPU_HAS_LOCK(&lLock) )
		return ;	// Nested!
	SHORTLOCK(&lLock);
	#if ARCHDIR_is_x86
	__asm__ __volatile__ ("sti");	// Start interrupts (x86 specific)
	#endif

	// Cache packets until a newline
	static char	cache[1500 - (sizeof(pkt_hdr) + 4)];
	static int	cache_len;
	
	 int	len = strlen(Text);

	// Append to cache
	strncpy(cache + cache_len, Text, sizeof(cache) - cache_len);
	cache_len += len;
	// TODO: Detect overflows.
	
	// If there's no newline, only buffer
	if( strpos(Text, '\n') == -1 ) {
		SHORTREL(&lLock);
		return ;
	}

	// Build packet
	 int	link_checksum_ofs = sizeof(pkt_hdr) + cache_len;
	char	buffer[sizeof(pkt_hdr) + cache_len + 4];

	memcpy(buffer, pkt_hdr, sizeof(pkt_hdr));
	memcpy(buffer + sizeof(pkt_hdr), cache, cache_len);
	
	((Uint16*)buffer)[(14+2)/2] = BigEndian16( sizeof(pkt_hdr)-14 + cache_len );	// IP Size
	((Uint16*)buffer)[(14+10)/2] = BigEndian16( 0 );	// IP Header
	((Uint16*)buffer)[(14+20+4)/2] = BigEndian16( 8+cache_len );	// UDP Size
	((Uint16*)buffer)[(14+20+6)/2] = BigEndian16( 0 );	// UDP Checksum
//	*(Uint32*)&buffer[link_checksum_ofs] = BigEndian32( 0 );	// 802.3 checksum?
	// TODO: Calculate checksums
	((Uint16*)buffer)[(14+10)/2] = BigEndian16( IPv4_Checksum(buffer+14,20) );	// IP Header
	
	// Create buffer
	tIPStackBuffer	*buf = IPStack_Buffer_CreateBuffer(1);
	IPStack_Buffer_AppendSubBuffer(buf, link_checksum_ofs+4, 0, buffer, NULL, NULL);

	// Send 'er off
	for( tAdapter *a = gpIP_AdapterList; a; a = a->Next )
	{
		a->Type->SendPacket( a->CardHandle, buf );
	}

	IPStack_Buffer_DestroyBuffer(buf);

	cache_len = 0;

	SHORTREL(&lLock);
}

