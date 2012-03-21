/*
 * Acess2 IP Stack
 * - Link/Media Layer Interface
 */
#include "ipstack.h"
#include "link.h"
#include "include/buffer.h"

// === CONSTANTS ===
#define	MAX_PACKET_SIZE	2048

// === PROTOTYPES ===
void	Link_RegisterType(Uint16 Type, tPacketCallback Callback);
void	Link_SendPacket(tAdapter *Adapter, Uint16 Type, tMacAddr To, tIPStackBuffer *Buffer);
void	Link_WatchDevice(tAdapter *Adapter);
// --- CRC ---
void	Link_InitCRC(void);
Uint32	Link_CalculateCRC(tIPStackBuffer *Buffer);
Uint32	Link_CalculatePartialCRC(Uint32 CRC, const void *Data, int Length);

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
	
	for( i = giRegisteredTypes; i -- ; )
	{
		if(gaRegisteredTypes[i].Type == Type) {
			Log_Warning("Net Link", "Attempt to register 0x%x twice", Type);
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
			Log_Warning("Net Link",
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
void Link_SendPacket(tAdapter *Adapter, Uint16 Type, tMacAddr To, tIPStackBuffer *Buffer)
{
	 int	length = IPStack_Buffer_GetLength(Buffer);
	 int	ofs = 4 - (length & 3);
	Uint8	buf[sizeof(tEthernetHeader) + ofs + 4];
	tEthernetHeader	*hdr = (void*)buf;

	if( ofs == 4 )	ofs = 0;	

	Log_Log("Net Link", "Sending %i bytes to %02x:%02x:%02x:%02x:%02x:%02x (Type 0x%x)",
		length, To.B[0], To.B[1], To.B[2], To.B[3], To.B[4], To.B[5], Type);

	hdr->Dest = To;
	hdr->Src = Adapter->MacAddr;
	hdr->Type = htons(Type);
	*(Uint32*)(buf + sizeof(tEthernetHeader) + ofs) = 0;

	IPStack_Buffer_AppendSubBuffer(Buffer, sizeof(tEthernetHeader), ofs + 4, hdr, NULL, NULL);
	
	*(Uint32*)(buf + sizeof(tEthernetHeader) + ofs) = htonl( Link_CalculateCRC(Buffer) );

	size_t outlen = 0;
	void *data = IPStack_Buffer_CompactBuffer(Buffer, &outlen);
	VFS_Write(Adapter->DeviceFD, outlen, data);
	free(data);
}

void Link_WorkerThread(void *Ptr)
{
	tAdapter	*Adapter = Ptr;
	
	Threads_SetName(Adapter->Device);
	Log_Log("Net Link", "Thread %i watching '%s'", Threads_GetTID(), Adapter->Device);

	// Child Thread
	while(Adapter->DeviceFD != -1)
	{
		Uint8	buf[MAX_PACKET_SIZE];
		tEthernetHeader	*hdr = (void*)buf;
		 int	ret, i;
		Uint32	checksum;
		
		// Wait for a packet (Read on a network device is blocking)
		//Log_Debug("NET", "Waiting on adapter FD#0x%x", Adapter->DeviceFD);
		ret = VFS_Read(Adapter->DeviceFD, MAX_PACKET_SIZE, buf);
		if(ret == -1)	break;
		
		if(ret < sizeof(tEthernetHeader)) {
			Log_Log("Net Link", "Recieved an undersized packet (%i < %i)",
				ret, sizeof(tEthernetHeader));
			continue;
		}
		
		Log_Log("Net Link",
			"Packet from %02x:%02x:%02x:%02x:%02x:%02x"
			" to %02x:%02x:%02x:%02x:%02x:%02x (Type=%04x)",
			hdr->Src.B[0], hdr->Src.B[1], hdr->Src.B[2],
			hdr->Src.B[3], hdr->Src.B[4], hdr->Src.B[5],
			hdr->Dest.B[0], hdr->Dest.B[1], hdr->Dest.B[2],
			hdr->Dest.B[3], hdr->Dest.B[4], hdr->Dest.B[5],
			ntohs(hdr->Type)
			);
		checksum = *(Uint32*)&hdr->Data[ret-sizeof(tEthernetHeader)-4];
		//Log_Log("NET", "Checksum 0x%08x", checksum);
		// TODO: Check checksum
		
		// Check if there is a registered callback for this packet type
		for( i = giRegisteredTypes; i--; )
		{
			if(gaRegisteredTypes[i].Type == ntohs(hdr->Type))	break;
		}
		// No? Ignore it
		if( i == -1 ) {
			Log_Log("Net Link", "Unregistered type 0x%x", ntohs(hdr->Type));
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
	
	Log_Log("Net Link", "Watcher terminated (file closed)");
	
	Threads_Exit(0, 0);
}

/**
 * \fn void Link_WatchDevice(tAdapter *Adapter)
 * \brief Spawns a worker thread to watch the specified adapter
 */
void Link_WatchDevice(tAdapter *Adapter)
{
	 int	tid;

	if( !gbLink_CRCTableGenerated )
		Link_InitCRC();
	
	tid = Proc_SpawnWorker(Link_WorkerThread, Adapter);	// Create a new worker thread
	
	if(tid < 0) {
		Log_Warning("Net Link", "Unable to create watcher thread for '%s'", Adapter->Device);
		return ;
	}
	
	Log_Log("Net Link", "Watching '%s' using tid %i", Adapter->Device, tid);
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

Uint32 Link_CalculateCRC(tIPStackBuffer *Buffer)
{
	Uint32	ret = 0xFFFFFFFF;
	const void	*data;
	size_t	length;

	 int	id = -1;
	while( (id = IPStack_Buffer_GetBuffer(Buffer, id, &length, &data)) != -1 )
	{
		ret = Link_CalculatePartialCRC(ret, data, length);
	}

	return ~ret;
}

Uint32 Link_CalculatePartialCRC(Uint32 CRC, const void *Data, int Length)
{
	// x32 + x26 + x23 + x22 + x16 + x12 + x11 + x10 + x8 + x7 + x5 + x4 + x2 + x + 1
	const Uint32	*data = Data;

	for( int i = 0; i < Length/4; i++ )
	{
		CRC = (CRC << 8 | *data++) ^ gaiLink_CRCTable[CRC >> 24];
	}

	return CRC;
}
