/*
 * Acess2 VIA Rhine II Driver (VT6102)
 * - By John Hodge (thePowersGang)
 */
#define	DEBUG	0
#define VERSION	((0<<8)|10)
#include <acess.h>
#include <modules.h>
#include <fs_devfs.h>
#include <drv_pci.h>
#include <api_drv_network.h>
#include <semaphore.h>

// === CONSTANTS ===
#define VENDOR_ID	0x1106
#define DEVICE_ID	0x3065

enum eRegs
{
	REG_PAR0, REG_PAR1,
	REG_PAR2, REG_PAR3,
	REG_PAR4, REG_PAR5,
	REG_RCR,  REG_TCR,
	REG_CR0,  REG_CR1,
	REG_rsv0, REG_rsv1,
	REG_ISR0, REG_ISR1,
	REG_IMR0, REG_IMR1,
};

// === TYPES ===
typedef struct sCard
{
	Uint16	IOBase;
	Uint8	IRQ;
	
	 int	NumWaitingPackets;
	
	char	Name[2];
	tVFS_Node	Node;
	Uint8	MacAddr[6];
}	tCard;

// === PROTOTYPES ===
 int	Rhine2_Install(char **Options);
char	*Rhine2_ReadDir(tVFS_Node *Node, int Pos);
tVFS_Node	*Rhine2_FindDir(tVFS_Node *Node, const char *Filename);
 int	Rhine2_RootIOCtl(tVFS_Node *Node, int ID, void *Arg);
Uint64	Rhine2_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
Uint64	Rhine2_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer);
 int	Rhine2_IOCtl(tVFS_Node *Node, int ID, void *Arg);
void	Rhine2_IRQHandler(int Num);

// === GLOBALS ===
MODULE_DEFINE(0, VERSION, VIARhineII, Rhine2_Install, NULL, NULL);
tVFS_NodeType	gRhine2_DirType = {
	.ReadDir = Rhine2_ReadDir,
	.FindDir = Rhine2_FindDir,
	.IOCtl = Rhine2_RootIOCtl
	};
tDevFS_Driver	gRhine2_DriverInfo = {
	NULL, "Rhine2",
	{
	.NumACLs = 1,
	.ACLs = &gVFS_ACL_EveryoneRX,
	.Flags = VFS_FFLAG_DIRECTORY,
	.Type = &gRhine2_DirType
	}
};
 int	giRhine2_CardCount;
tCard	*gaRhine2_Cards;

// === CODE ===
/**
 * \brief Installs the PCnet3 Driver
 */
int Rhine2_Install(char **Options)
{
	 int	id = -1;
	 int	i = 0;
//	Uint16	base;
	tCard	*card;
	
	giRhine2_CardCount = PCI_CountDevices(VENDOR_ID, DEVICE_ID);
	Log_Debug("PCnet3", "%i cards", giRhine2_CardCount);
	
	if( giRhine2_CardCount == 0 )	return MODULE_ERR_NOTNEEDED;
	
	gaRhine2_Cards = calloc( giRhine2_CardCount, sizeof(tCard) );
	
	while( (id = PCI_GetDevice(VENDOR_ID, DEVICE_ID, i)) != -1 )
	{
		card = &gaRhine2_Cards[i];
		
		LOG("BAR0 = 0x%08x", PCI_GetBAR(id, 0));
		LOG("BAR1 = 0x%08x", PCI_GetBAR(id, 1));
		LOG("BAR2 = 0x%08x", PCI_GetBAR(id, 2));
		LOG("BAR3 = 0x%08x", PCI_GetBAR(id, 3));
		LOG("BAR4 = 0x%08x", PCI_GetBAR(id, 4));
		LOG("BAR5 = 0x%08x", PCI_GetBAR(id, 5));
		
//		card->IOBase = base;
//		card->IRQ = PCI_GetIRQ( id );
		
		// Install IRQ Handler
//		IRQ_AddHandler(card->IRQ, Rhine2_IRQHandler);
		
		
		
//		Log_Log("PCnet3", "Card %i 0x%04x, IRQ %i %02x:%02x:%02x:%02x:%02x:%02x",
//			i, card->IOBase, card->IRQ,
//			card->MacAddr[0], card->MacAddr[1], card->MacAddr[2],
//			card->MacAddr[3], card->MacAddr[4], card->MacAddr[5]
//			);
		
		i ++;
	}
	
	gRhine2_DriverInfo.RootNode.Size = giRhine2_CardCount;
	DevFS_AddDevice( &gRhine2_DriverInfo );
	
	return MODULE_ERR_OK;
}

// --- Root Functions ---
char *Rhine2_ReadDir(tVFS_Node *Node, int Pos)
{
	if( Pos < 0 || Pos >= giRhine2_CardCount )	return NULL;
	
	return strdup( gaRhine2_Cards[Pos].Name );
}

tVFS_Node *Rhine2_FindDir(tVFS_Node *Node, const char *Filename)
{
	//TODO: It might be an idea to supprt >10 cards
	if(Filename[0] == '\0' || Filename[1] != '\0')	return NULL;
	if(Filename[0] < '0' || Filename[0] > '9')	return NULL;
	return &gaRhine2_Cards[ Filename[0]-'0' ].Node;
}

const char *csaRhine2_RootIOCtls[] = {DRV_IOCTLNAMES, NULL};
int Rhine2_RootIOCtl(tVFS_Node *Node, int ID, void *Data)
{
	ENTER("pNode iID pData", Node, ID, Data);
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_NETWORK, "PCnet3", VERSION, csaRhine2_RootIOCtls);
	}
	LEAVE('i', 0);
	return 0;
}

// --- File Functions ---
Uint64 Rhine2_Read(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	#if 0
	tCard	*card = Node->ImplPtr;
	Uint16	read_ofs, pkt_length;
	 int	new_read_ofs;

	ENTER("pNode XOffset XLength pBuffer", Node, Offset, Length, Buffer);

retry:
	if( Semaphore_Wait( &card->ReadSemaphore, 1 ) != 1 )
	{
		LEAVE_RET('i', 0);
	}
	
	Mutex_Acquire( &card->ReadMutex );
	
	read_ofs = inw( card->IOBase + CAPR );
	LOG("raw read_ofs = %i", read_ofs);
	read_ofs = (read_ofs + 0x10) & 0xFFFF;
	LOG("read_ofs = %i", read_ofs);
	
	pkt_length = *(Uint16*)&card->ReceiveBuffer[read_ofs+2];
	
	// Calculate new read offset
	new_read_ofs = read_ofs + pkt_length + 4;
	new_read_ofs = (new_read_ofs + 3) & ~3;	// Align
	if(new_read_ofs > card->ReceiveBufferLength) {
		LOG("wrapping read_ofs");
		new_read_ofs -= card->ReceiveBufferLength;
	}
	new_read_ofs -= 0x10;	// I dunno
	LOG("new_read_ofs = %i", new_read_ofs);
	
	// Check for errors
	if( *(Uint16*)&card->ReceiveBuffer[read_ofs] & 0x1E ) {
		// Update CAPR
		outw(card->IOBase + CAPR, new_read_ofs);
		Mutex_Release( &card->ReadMutex );
		goto retry;	// I feel evil
	}
	
	// Get packet
	if( Length > pkt_length )	Length = pkt_length;
	memcpy(Buffer, &card->ReceiveBuffer[read_ofs+4], Length);
	
	// Update CAPR
	outw(card->IOBase + CAPR, new_read_ofs);
	
	Mutex_Release( &card->ReadMutex );
	
	LEAVE('i', Length);
	#endif
	
	return Length;
}

Uint64 Rhine2_Write(tVFS_Node *Node, Uint64 Offset, Uint64 Length, void *Buffer)
{
	#if 0
	 int	td;
	Uint32	status;
	tCard	*card = Node->ImplPtr;
	
	if( Length > 1500 )	return 0;	// MTU exceeded
	
	ENTER("pNode XLength pBuffer", Node, Length, Buffer);
	
	// TODO: Implement a semaphore for avaliable transmit buffers

	// Find an avaliable descriptor
	Mutex_Acquire(&card->CurTXProtector);
	td = card->CurTXDescriptor;
	card->CurTXDescriptor ++;
	card->CurTXDescriptor %= 4;
	Mutex_Release(&card->CurTXProtector);
	// - Lock it
	Mutex_Acquire( &card->TransmitInUse[td] );
	
	LOG("td = %i", td);
	
	// Transmit using descriptor `td`
	LOG("card->PhysTransmitBuffers[td] = %P", card->PhysTransmitBuffers[td]);
	outd(card->IOBase + TSAD0 + td*4, card->PhysTransmitBuffers[td]);
	LOG("card->TransmitBuffers[td] = %p", card->TransmitBuffers[td]);
	// Copy to buffer
	memcpy(card->TransmitBuffers[td], Buffer, Length);
	// Start
	status = 0;
	status |= Length & 0x1FFF;	// 0-12: Length
	status |= 0 << 13;	// 13: OWN bit
	status |= (0 & 0x3F) << 16;	// 16-21: Early TX threshold (zero atm, TODO: check)
	LOG("status = 0x%08x", status);
	outd(card->IOBase + TSD0 + td*4, status);
	
	LEAVE('i', (int)Length);
	#endif
	
	return Length;
}

const char *csaRhine2_NodeIOCtls[] = {DRV_IOCTLNAMES, NULL};
int Rhine2_IOCtl(tVFS_Node *Node, int ID, void *Data)
{
	tCard	*card = Node->ImplPtr;
	ENTER("pNode iID pData", Node, ID, Data);
	switch(ID)
	{
	BASE_IOCTLS(DRV_TYPE_NETWORK, "PCnet3", VERSION, csaRhine2_NodeIOCtls);
	case NET_IOCTL_GETMAC:
		if( !CheckMem(Data, 6) ) {
			LEAVE('i', -1);
			return -1;
		}
		memcpy( Data, card->MacAddr, 6 );
		LEAVE('i', 1);
		return 1;
	}
	LEAVE('i', 0);
	return 0;
}

void Rhine2_IRQHandler(int Num)
{
	#if 0
	 int	i, j;
	tCard	*card;
	Uint16	status;

	LOG("Num = %i", Num);
	
	for( i = 0; i < giRhine2_CardCount; i ++ )
	{
		card = &gaRhine2_Cards[i];
		if( Num != card->IRQ )	break;
		
		status = inw(card->IOBase + ISR);
		LOG("status = 0x%02x", status);
		
		// Transmit OK, a transmit descriptor is now free
		if( status & FLAG_ISR_TOK )
		{
			for( j = 0; j < 4; j ++ )
			{
				if( ind(card->IOBase + TSD0 + j*4) & 0x8000 ) {	// TSD TOK
					Mutex_Release( &card->TransmitInUse[j] );
					// TODO: Update semaphore once implemented
				}
			}
			outw(card->IOBase + ISR, FLAG_ISR_TOK);
		}
		
		// Recieve OK, inform read
		if( status & FLAG_ISR_ROK )
		{
			 int	read_ofs, end_ofs;
			 int	packet_count = 0;
			 int	len;
			
			// Scan recieve buffer for packets
			end_ofs = inw(card->IOBase + CBA);
			read_ofs = card->SeenOfs;
			LOG("read_ofs = %i, end_ofs = %i", read_ofs, end_ofs);
			if( read_ofs > end_ofs )
			{
				while( read_ofs < card->ReceiveBufferLength )
				{
					packet_count ++;
					len = *(Uint16*)&card->ReceiveBuffer[read_ofs+2];
					LOG("%i 0x%x Pkt Hdr: 0x%04x, len: 0x%04x",
						packet_count, read_ofs,
						*(Uint16*)&card->ReceiveBuffer[read_ofs],
						len
						);
					if(len > 2000) {
						Log_Warning("PCnet3", "IRQ: Packet in buffer exceeds sanity (%i>2000)", len);
					}
					read_ofs += len + 4;
					read_ofs = (read_ofs + 3) & ~3;	// Align
				}
				read_ofs -= card->ReceiveBufferLength;
				LOG("wrapped read_ofs");
			}
			while( read_ofs < end_ofs )
			{
				packet_count ++;
				LOG("%i 0x%x Pkt Hdr: 0x%04x, len: 0x%04x",
					packet_count, read_ofs,
					*(Uint16*)&card->ReceiveBuffer[read_ofs],
					*(Uint16*)&card->ReceiveBuffer[read_ofs+2]
					);
				read_ofs += *(Uint16*)&card->ReceiveBuffer[read_ofs+2] + 4;
				read_ofs = (read_ofs + 3) & ~3;	// Align
			}
			if( read_ofs != end_ofs ) {
				Log_Warning("PCnet3", "IRQ: read_ofs (%i) != end_ofs(%i)", read_ofs, end_ofs);
				read_ofs = end_ofs;
			}
			card->SeenOfs = read_ofs;
			
			LOG("packet_count = %i, read_ofs = 0x%x", packet_count, read_ofs);
			
			if( packet_count )
			{
				if( Semaphore_Signal( &card->ReadSemaphore, packet_count ) != packet_count ) {
					// Oops?
				}
				VFS_MarkAvaliable( &card->Node, 1 );
			}
			
			outw(card->IOBase + ISR, FLAG_ISR_ROK);
		}
	}
	#endif
}
