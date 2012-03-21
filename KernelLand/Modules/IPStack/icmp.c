/*
 * Acess2 IP Stack
 * - ICMP Handling
 */
#include "ipstack.h"
#include "ipv4.h"
#include "icmp.h"

// === CONSTANTS ===
#define PING_SLOTS	64

// === PROTOTYPES ===
void	ICMP_Initialise();
void	ICMP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer);

// === GLOBALS ===
struct {
	tInterface	*Interface;
	 int	bArrived;
}	gICMP_PingSlots[PING_SLOTS];

// === CODE ===
/**
 * \fn void ICMP_Initialise()
 * \brief Initialise the ICMP Layer
 */
void ICMP_Initialise()
{
	IPv4_RegisterCallback(IP4PROT_ICMP, ICMP_GetPacket);
}

/**
 * \fn void ICMP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
 * \brief Handles a packet from the IP Layer
 */
void ICMP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
{
	tICMPHeader	*hdr = Buffer;
	
	//Log_Debug("ICMPv4", "Length = %i", Length);
	Log_Debug("ICMPv4", "hdr->Type, hdr->Code = %i, %i", hdr->Type, hdr->Code);
	//Log_Debug("ICMPv4", "hdr->Checksum = 0x%x", ntohs(hdr->Checksum));
	Log_Debug("ICMPv4", "hdr->ID = 0x%x", ntohs(hdr->ID));
	Log_Debug("ICMPv4", "hdr->Sequence = 0x%x", ntohs(hdr->Sequence));
	
	switch(hdr->Type)
	{
	// -- 0: Echo Reply
	case ICMP_ECHOREPLY:
		if(hdr->Code != 0) {
			Log_Warning("ICMPv4", "Code == %i for ICMP Echo Reply, should be 0", hdr->Code);
			return ;
		}
		if(hdr->ID != (Uint16)~hdr->Sequence) {
			Log_Warning("ICMPv4", "ID and Sequence values do not match");
			//return ;
		}
		gICMP_PingSlots[hdr->ID].bArrived = 1;
		break;
	
	// -- 3: Destination Unreachable
	case ICMP_UNREACHABLE:
		switch(hdr->Code)
		{
		case 3:	// Port Unreachable
			Log_Debug("ICMPv4", "Destination Unreachable (Port Unreachable)");
			break;
		default:
			Log_Debug("ICMPv4", "Destination Unreachable (Code %i)", hdr->Code);
			break;
		}
//		IPv4_Unreachable( Interface, hdr->Code, htons(hdr->Length)-sizeof(tICMPHeader), hdr->Data );
		break;
	
	// -- 8: Echo Request
	case ICMP_ECHOREQ:
		if(hdr->Code != 0) {
			Log_Warning("ICMPv4", "Code == %i for ICMP Echo Request, should be 0", hdr->Code);
			return ;
		}
		//Log_Debug("ICMPv4", "Replying");
		hdr->Type = ICMP_ECHOREPLY;
		hdr->Checksum = 0;
		hdr->Checksum = htons( IPv4_Checksum( (Uint16*)hdr, Length/2 ) );
		//Log_Debug("ICMPv4", "Checksum = 0x%04x", hdr->Checksum);
		
		tIPStackBuffer	*buffer = IPStack_Buffer_CreateBuffer(1 + IPV4_BUFFERS);
		IPStack_Buffer_AppendSubBuffer(buffer, Length, 0, hdr, NULL, NULL);
		IPv4_SendPacket(Interface, *(tIPv4*)Address, 1, ntohs(hdr->Sequence), buffer);
		IPStack_Buffer_DestroyBuffer(buffer);
		break;
	default:
		break;
	}
	
}

/**
 * \brief Sends ICMP Echo and waits for the reply
 * \note Times out after \a Interface->TimeoutDelay has elapsed
 */
int ICMP_Ping(tInterface *Interface, tIPv4 Addr)
{
	Sint64	ts;
	Sint64	end;
	char	buf[32] = "\x8\0\0\0\0\0\0\0Acess2 I"
                      "P/TCP Stack 1.0\0";
	tICMPHeader	*hdr = (void*)buf;
	 int	i;
	
	for(;;)
	{
		for(i=0;i<PING_SLOTS;i++)
		{
			if(gICMP_PingSlots[i].Interface == NULL)	break;
		}
		if( i < PING_SLOTS )	break;
		Threads_Yield();
	}
	gICMP_PingSlots[i].Interface = Interface;
	gICMP_PingSlots[i].bArrived = 0;
	hdr->ID = i;
	hdr->Sequence = ~i;
	hdr->Checksum = htons( IPv4_Checksum((Uint16*)hdr, sizeof(buf)/2) );
	
	ts = now();
	
	tIPStackBuffer	*buffer = IPStack_Buffer_CreateBuffer(1 + IPV4_BUFFERS);
	IPStack_Buffer_AppendSubBuffer(buffer, sizeof(buf), 0, buf, NULL, NULL);
	IPv4_SendPacket(Interface, Addr, 1, i, buffer);
	IPStack_Buffer_DestroyBuffer(buffer);
	
	end = ts + Interface->TimeoutDelay;
	while( !gICMP_PingSlots[i].bArrived && now() < end)	Threads_Yield();
	
	if(now() > end)
		return -1;
	
	return (int)( now() - ts );
}
