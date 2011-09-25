/*
 * DNS Resolver
 */
#include <acess/sys.h>
#include <stdio.h>
#include <net.h>

// === CONSTANTS ===
#define NAMESERVER	{10,0,2,3}

uint8_t	packet[] = {
		// --- UDP header
		53, 0,	// Dest port
		4, 0,	// IPv4
		10,0,2,3,	// Dest address
		// --- DNS Packet
		0x13,0x37,
		0x01,	// Recursion Desired
		0x00,
		0,1,	// QDCOUNT
		0,0,
		0,0,
		0,0,	// ARCOUNT
		
		#if 0
		// www.mutabah.net, A, IN
		3,'w','w','w',
		7,'m','u','t','a','b','a','h',
		3,'n','e','t',
		#else
		// mussel.ucc.asn.au, A, IN
		6,'m','u','s','s','e','l',
		3,'u','c','c',
		3,'a','s','n',
		2,'a','u',
		#endif
		0,
		0,1,	// A
		0,1		// IN
	};

// === TYPES ===
typedef struct sDNSPacket
{
	uint16_t	ID;
	// 1: Query/Response, 4: Opcode, 1: Authorative Answer, 1: Truncation, 1: Recursion Denied
	uint8_t	opcode;
	uint8_t	rcode;	// 1: Recusions Desired, 3: Zero, 4: Response Code
	uint16_t	qdcount;	// Question Count
	uint16_t	ancount;	// Answer Count
	uint16_t	nscount;	// Nameserver Resource Count
	uint16_t	arcount;	// Additional Records count
}	tDNSPacket;

// === PROTOTYPES ===

// ==== CODE ====
int main(int argc, char *argv[], char *envp[])
{
	 int	fd, tmp;
	uint16_t	port = 53;
	uint8_t 	destip[4] = NAMESERVER;
	uint8_t 	buf[512];
	tDNSPacket	*hdr = (void *) (buf + 4 + 4);

	fd = Net_OpenSocket(4, destip, "udp");
	if( fd == -1 ) {
		fprintf(stderr, "Unable to create a UDP socket\n");
		return 0;
	}
	port = 0;	ioctl(fd, ioctl(fd, 3, "getset_localport"), &port);
	port = 53;	ioctl(fd, ioctl(fd, 3, "getset_remoteport"), &port);
	ioctl(fd, ioctl(fd, 3, "set_remoteaddr"), &destip);
	tmp = 32;	ioctl(fd, ioctl(fd, 3, "getset_remotemask"), &tmp);
	
	write(fd, sizeof(packet), packet);
	read(fd, 512, buf);
	
	printf("hdr = {\n");
	printf("  .ID = 0x%04x\n", hdr->ID);
	printf("  .opcode = 0x%04x\n", hdr->opcode);
	printf("  .rcode = 0x%02x\n", hdr->rcode);
	printf("  .qdcount = 0x%04x\n", hdr->qdcount);
	printf("  .ancount = 0x%04x\n", hdr->ancount);
	printf("  .nscount = 0x%04x\n", hdr->nscount);
	printf("  .arcount = 0x%04x\n", hdr->arcount);
	printf("}\n");
	
	
	close(fd);
	return 0;
}
