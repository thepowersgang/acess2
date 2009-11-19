/*
 * Acess2 IP Stack
 * - Link/Media Layer Interface
 */
#include "ipstack.h"
#include "link.h"

// === CONSTANTS ===
#define	MAX_PACKET_SIZE	2048

// === GLOBALS ===
 int	giRegisteredTypes = 0;
struct {
	Uint16	Type;
	tPacketCallback	Callback;
}	*gaRegisteredTypes;

// === CODE ===
/**
 * \fn void Link_RegisterType(Uint16 Type, tPacketCallback Callback)
 * \brief Registers a callback for a specific packet type
 * 
 * \todo Make thread safe (place a mutex on the list)
 */
void Link_RegisterType(Uint16 Type, tPacketCallback Callback)
{
	 int	i;
	void	*tmp;
	for( i = giRegisteredTypes; i -- ; )
	{
		if(gaRegisteredTypes[i].Type == Type) {
			Warning("[NET  ] Attempt to register 0x%x twice", Type);
			return ;
		}
		// Ooh! Free slot!
		if(gaRegisteredTypes[i].Callback == NULL)	break;
	}
	
	if(i == -1)
	{
		tmp = realloc(gaRegisteredTypes, (giRegisteredTypes+1)*sizeof(*gaRegisteredTypes));
		if(!tmp) {
			Warning("[NET  ] Out of heap space!");
			return ;
		}
		i = giRegisteredTypes;
		giRegisteredTypes ++;
		gaRegisteredTypes = tmp;
	}
	
	gaRegisteredTypes[i].Callback = Callback;
	gaRegisteredTypes[i].Type = Type;
}

/**
 * \fn void Link_SendPacket(tAdapter *Adapter, Uint16 Type, tMacAddr To, int Length, void *Buffer)
 * \brief Formats and sends a packet on the specified interface
 */
void Link_SendPacket(tAdapter *Adapter, Uint16 Type, tMacAddr To, int Length, void *Buffer)
{
	 int	bufSize = sizeof(tEthernetHeader) + Length;
	Uint8	buf[bufSize];
	tEthernetHeader	*hdr = (void*)buf;
	
	hdr->Dest = To;
	hdr->Src = Adapter->MacAddr;
	hdr->Type = htons(Type);
	
	memcpy(hdr->Data, Buffer, Length);
	
	VFS_Write(Adapter->DeviceFD, bufSize, buf);
}

/**
 * \fn void Link_WatchDevice(tAdapter *Adapter)
 * \brief Spawns a worker thread to watch the specified adapter
 */
void Link_WatchDevice(tAdapter *Adapter)
{
	 int	tid = Proc_SpawnWorker();	// Create a new worker thread
	
	if(tid < 0) {
		Warning("[NET  ] Unable to create watcher thread for '%s'", Adapter->Device);
		return ;
	}
	
	if(tid > 0) {
		Log("[NET  ] Watching '%s' using tid %i", Adapter->Device, tid);
		return ;
	}
	
	// Child Thread
	while(Adapter->DeviceFD != -1)
	{
		Uint8	buf[MAX_PACKET_SIZE];
		tEthernetHeader	*hdr = (void*)buf;
		 int	ret, i;
		
		// Wait for a packet (Read on a network device is blocking)
		ret = VFS_Read(Adapter->DeviceFD, MAX_PACKET_SIZE, buf);
		if(ret == -1)	break;
		
		if(ret <= sizeof(tEthernetHeader)) {
			Log("[NET  ] Recieved an undersized packet");
			continue;
		}
		
		// Check if there is a registered callback for this packet type
		for( i = giRegisteredTypes; i--; )
		{
			if(gaRegisteredTypes[i].Type == hdr->Type)	continue;
		}
		// No? Ignore it
		if( i == -1 )	continue;
		
		// Call the callback
		gaRegisteredTypes[i].Callback(
			Adapter,
			hdr->Src,
			ret - sizeof(tEthernetHeader),
			hdr->Data
			);
	}
	
	Log("[NET  ] Watcher terminated (file closed)");
}
