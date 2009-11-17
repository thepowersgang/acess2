/*
 * Acess2 IP Stack
 * - ICMP Handling
 */
#include "ipstack.h"
#include "icmp.h"

// === PROTOTYPES ===
void	ICMP_Initialise();
void	ICMP_GetPacket(tInterface *Interface, int Length, void *Buffer);

// === GLOBALS ===

// === CODE ===
/**
 * \fn void ICMP_Initialise()
 * \brief Initialise the ICMP Layer
 */
void ICMP_Initialise()
{
	
}

/**
 * \fn void ICMP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
 * \brief Handles a packet from the IP Layer
 */
void ICMP_GetPacket(tInterface *Interface, void *Address, int Length, void *Buffer)
{
	//tICMPHeader	*hdr = Buffer;
	
}
