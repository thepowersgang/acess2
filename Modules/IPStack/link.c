/*
 * Acess2 IP Stack
 * - Link/Media Layer Interface
 */
#include "ipstack.h"
#include "link.h"

// === CONSTANTS ===
#define	MAX_PACKET_SIZE	2048

// === PROTOTYPES ===
void	Link_RegisterType(Uint16 Type, tPacketCallback Callback);
void	Link_InitCRC();
Uint32	Link_CalculateCRC(void *Data, int Length);
void	Link_SendPacket(tAdapter *Adapter, Uint16 Type, tMacAddr To, int Length, void *Buffer);
void	Link_WatchDevice(tAdapter *Adapter);

// === GLOBALS ===
 int	giRegisteredTypes = 0;
 int	giRegisteredTypeSpace = 0;
struct {
	Uint16	Type;
	tPacketCallback	Callback;
}	*gaRegisteredTypes;
 int	gbLink_CRCTableGenerated = 0;
Uint32	gaiLink_CRCTable[256];

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
	
	Type = htons(Type);	// Set to network order
	
	for( i = giRegisteredTypes; i -- ; )
	{
		if(gaRegisteredTypes[i].Type == Type) {
			Log_Warning("NET", "Attempt to register 0x%x twice", Type);
			return ;
		}
		// Ooh! Free slot!
		if(gaRegisteredTypes[i].Callback == NULL)	break;
	}
	
	if(i == -1)
	{
		giRegisteredTypeSpace += 5;
		tmp = realloc(gaRegisteredTypes, giRegisteredTypeSpace*sizeof(*gaRegisteredTypes));
		if(!tmp) {
			Log_Warning("NET",
				"Out of heap space! (Attempted to allocate %i)",
				giRegisteredTypeSpace*sizeof(*gaRegisteredTypes)
				);
			return ;
		}
		gaRegisteredTypes = tmp;
		i = giRegisteredTypes;
		giRegisteredTypes ++;
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
	 int	bufSize = sizeof(tEthernetHeader) + ((Length+3)&~3) + 4;
	Uint8	buf[bufSize];	// dynamic stack arrays ftw!
	tEthernetHeader	*hdr = (void*)buf;
	
	Log_Log("NET", "Sending %i bytes to %02x:%02x:%02x:%02x:%02x:%02x (Type 0x%x)",
		Length, To.B[0], To.B[1], To.B[2], To.B[3], To.B[4], To.B[5], Type);
	
	hdr->Dest = To;
	hdr->Src = Adapter->MacAddr;
	hdr->Type = htons(Type);
	
	memcpy(hdr->Data, Buffer, Length);
	
	*(Uint32*) &hdr->Data[bufSize-4] = 0;
	*(Uint32*) &hdr->Data[bufSize-4] = htonl( Link_CalculateCRC(buf, bufSize) );
	
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
		Log_Warning("NET", "Unable to create watcher thread for '%s'", Adapter->Device);
		return ;
	}
	
	if(tid > 0) {
		Log_Log("NET", "Watching '%s' using tid %i", Adapter->Device, tid);
		return ;
	}
	
	if( !gbLink_CRCTableGenerated )
		Link_InitCRC();
	
	// Child Thread
	while(Adapter->DeviceFD != -1)
	{
		Uint8	buf[MAX_PACKET_SIZE];
		tEthernetHeader	*hdr = (void*)buf;
		 int	ret, i;
		Uint32	checksum;
		
		// Wait for a packet (Read on a network device is blocking)
		ret = VFS_Read(Adapter->DeviceFD, MAX_PACKET_SIZE, buf);
		if(ret == -1)	break;
		
		if(ret <= (int)sizeof(tEthernetHeader)) {
			Log_Log("NET", "Recieved an undersized packet");
			continue;
		}
		
		Log_Log("NET", "Packet from %02x:%02x:%02x:%02x:%02x:%02x",
			hdr->Src.B[0], hdr->Src.B[1], hdr->Src.B[2],
			hdr->Src.B[3], hdr->Src.B[4], hdr->Src.B[5]
			);
		Log_Log("NET", "to %02x:%02x:%02x:%02x:%02x:%02x",
			hdr->Dest.B[0], hdr->Dest.B[1], hdr->Dest.B[2],
			hdr->Dest.B[3], hdr->Dest.B[4], hdr->Dest.B[5]
			);
		checksum = *(Uint32*)&hdr->Data[ret-sizeof(tEthernetHeader)-4];
		Log_Log("NET", "Checksum 0x%08x", checksum);
		
		// Check if there is a registered callback for this packet type
		for( i = giRegisteredTypes; i--; )
		{
			if(gaRegisteredTypes[i].Type == hdr->Type)	break;
		}
		// No? Ignore it
		if( i == -1 ) {
			Log_Log("NET", "Unregistered type 0x%x", ntohs(hdr->Type));
			continue;
		}
		
		// Call the callback
		gaRegisteredTypes[i].Callback(
			Adapter,
			hdr->Src,
			ret - sizeof(tEthernetHeader),
			hdr->Data
			);
	}
	
	Log_Log("NET", "Watcher terminated (file closed)");
	
	Threads_Exit(0, 0);
}

// From http://www.cl.cam.ac.uk/research/srg/bluebook/21/crc/node6.html
#define	QUOTIENT	0x04c11db7
void Link_InitCRC(void)
{
     int	i, j;
    Uint32	crc;

    for (i = 0; i < 256; i++)
    {
        crc = i << 24;
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x80000000)
                crc = (crc << 1) ^ QUOTIENT;
            else
                crc = crc << 1;
        }
        gaiLink_CRCTable[i] = crc;
    }
	
	gbLink_CRCTableGenerated = 1;
}

Uint32 Link_CalculateCRC(void *Data, int Length)
{
	// x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x + 1
	Uint32	result;
     int	i;
	Uint32	*data = Data;
    
    if(Length < 4)	return 0;

    result = *data++ << 24;
    result |= *data++ << 16;
    result |= *data++ << 8;
    result |= *data++;
    result = ~ result;
    Length -= 4;
    
    for( i = 0; i < Length; i++ )
    {
        result = (result << 8 | *data++) ^ gaiLink_CRCTable[result >> 24];
    }
    
    return ~result;
}
