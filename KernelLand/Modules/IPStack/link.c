/*
 * Acess2 Networking Stack
 * - By John Hodge (thePowersGang)
 *
 * link.c
 * - Ethernet/802.3 Handling code
 * TODO: Rename file
 */
#include "ipstack.h"
#include "link.h"
#include "include/buffer.h"
#include "include/adapters_int.h"

// === CONSTANTS ===
#define	MAX_PACKET_SIZE	2048

// === PROTOTYPES ===
void	Link_RegisterType(Uint16 Type, tPacketCallback Callback);
void	Link_SendPacket(tAdapter *Adapter, Uint16 Type, tMacAddr To, tIPStackBuffer *Buffer);
 int	Link_HandlePacket(tAdapter *Adapter, tIPStackBuffer *Buffer);
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
	 int	ofs = (4 - (length & 3)) & 3;
	Uint8	buf[sizeof(tEthernetHeader) + ofs + 4];
	Uint32	*checksum = (void*)(buf + sizeof(tEthernetHeader) + ofs);
	tEthernetHeader	*hdr = (void*)buf;

	Log_Log("Net Link", "Sending %i bytes to %02x:%02x:%02x:%02x:%02x:%02x (Type 0x%x)",
		length, To.B[0], To.B[1], To.B[2], To.B[3], To.B[4], To.B[5], Type);

	hdr->Dest = To;
	memcpy(&hdr->Src, Adapter->HWAddr, 6);	// TODO: Remove hard coded 6
	hdr->Type = htons(Type);
	memset(hdr, 0, ofs+4);	// zero padding and checksum

	if( (Adapter->Type->Flags & ADAPTERFLAG_OFFLOAD_MAC) )
		IPStack_Buffer_AppendSubBuffer(Buffer, sizeof(tEthernetHeader), ofs, hdr, NULL, NULL);
	else
	{
		IPStack_Buffer_AppendSubBuffer(Buffer, sizeof(tEthernetHeader), ofs + 4, hdr, NULL, NULL);
		*checksum = htonl( Link_CalculateCRC(Buffer) );
		Log_Debug("Net Link", "Non-Offloaded: 0x%x, 0x%x", *checksum, ntohl(*checksum));
	}

	Log_Log("Net Link", " from %02x:%02x:%02x:%02x:%02x:%02x",
		hdr->Src.B[0], hdr->Src.B[1], hdr->Src.B[2],
		hdr->Src.B[3], hdr->Src.B[4], hdr->Src.B[5]
		);

	Adapter_SendPacket(Adapter, Buffer);
}

int Link_HandlePacket(tAdapter *Adapter, tIPStackBuffer *Buffer)
{
	size_t	len = 0;
	void	*data = IPStack_Buffer_CompactBuffer(Buffer, &len);
	
	tEthernetHeader	*hdr = (void*)data;

	if(len < sizeof(tEthernetHeader)) {
		Log_Log("Net Link", "Recieved an undersized packet (%i < %i)",
			len, sizeof(tEthernetHeader));
		free(data);
		return 1;
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
//	Uint32 checksum = *(Uint32*)(data + len + 4);
	//Log_Log("NET", "Checksum 0x%08x", checksum);
	// TODO: Check checksum
	
	// Check if there is a registered callback for this packet type
	 int	i;
	for( i = giRegisteredTypes; i--; )
	{
		if(gaRegisteredTypes[i].Type == ntohs(hdr->Type))	break;
	}
	// No? Ignore it
	if( i == -1 ) {
		Log_Log("Net Link", "Unregistered type 0x%x", ntohs(hdr->Type));
		
		free(data);	
		return 1;
	}
	
	// Call the callback
	gaRegisteredTypes[i].Callback(
		Adapter,
		hdr->Src,
		len - sizeof(tEthernetHeader),
		hdr->Data
		);
	
	free(data);
	
	return 0;
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
