/*
 * 
 * http://www.ietf.org/rfc/rfc2131.txt
 */
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <net.h>

#define FILENAME_MAX	255
// --- Translation functions ---
static inline uint32_t htonl(uint32_t v)
{
	return	  (((v >> 24) & 0xFF) <<  0)
		| (((v >> 16) & 0xFF) <<  8)
		| (((v >>  8) & 0xFF) << 16)
		| (((v >>  0) & 0xFF) << 24);
}
static inline uint16_t htons(uint16_t v)
{
	return	  (((v >> 8) & 0xFF) <<  0)
		| (((v >> 0) & 0xFF) <<  8);
}
#define htonb(v)	v
#define ntohl(v)	htonl(v)
#define ntohs(v)	htons(v)

// === CONSTANTS ===
enum eStates
{
	STATE_PREINIT,
	STATE_DISCOVER_SENT,
	STATE_REQUEST_SENT,
	STATE_COMPLETE
};

// === STRUCTURES ===
#define DHCP_MAGIC	0x63825363
struct sDHCP_Message
{
	uint8_t	op;
	uint8_t	htype;	// 1 = Ethernet
	uint8_t	hlen;	// 6 bytes for MAC
	uint8_t	hops;	// Hop counting
	uint32_t	xid;
	uint16_t	secs;
	uint16_t	flags;
	uint32_t	ciaddr;
	uint32_t	yiaddr;
	uint32_t	siaddr;
	uint32_t	giaddr;
	uint8_t	chaddr[16];
	uint8_t	sname[64];
	uint8_t	file[128];
	uint32_t	dhcp_magic;	// 0x63 0x82 0x53 0x63
	uint8_t	options[];
};

typedef struct sInterface
{
	struct sInterface	*Next;
	char	*Adapter;
	 int	State;
	 int	Num;
	 int	SocketFD;
	 int	IfaceFD;

	uint32_t	TransactionID;
	char	HWAddr[6];
	
	int64_t	Timeout;
} tInterface;

// === PROTOTYPES ===
int	main(int argc, char *argv[]);
void	Scan_Dir(tInterface **IfaceList, const char *Directory);
int	Start_Interface(tInterface *Iface);
void	Send_DHCPDISCOVER(tInterface *Iface);
void	Send_DHCPREQUEST(tInterface *Iface, void *OfferBuffer, int TypeOffset);
int	Handle_Packet(tInterface *Iface);
void	Handle_Timeout(tInterface *Iface);
void	Update_State(tInterface *Iface, int newState);
void	SetAddress(tInterface *Iface, void *Addr, void *Mask, void *Router);

// === CODE ===
int main(int argc, char *argv[])
{
	tInterface	*ifaces = NULL, *i;

	// TODO: Scan /Devices and search for network adapters
	if( argc > 2 ) {
		fprintf(stderr, "Usage: %s <interface>\n", argv[0]);
		return -1;
	}
	
	if( argc == 2 ) {
		ifaces = malloc(sizeof(tInterface));
		ifaces->Next = NULL;
		ifaces->Adapter = argv[1];
	}
	else {
		Scan_Dir( &ifaces, "/Devices/ip/adapters" );
	}

	for( i = ifaces; i; i = i->Next )
	{
		if( Start_Interface(i) < 0 ) {
			return -1;
		}
		i->State = STATE_PREINIT;
		
		// Send request
		Send_DHCPDISCOVER(i);
	}

	while( ifaces ) 
	{
		 int	maxfd;
		fd_set	fds;
		tInterface	*p;
		int64_t	timeout = 1000;
	
		maxfd = 0;
		FD_ZERO(&fds);
		for( i = ifaces; i; i = i->Next )
		{
			FD_SET(i->SocketFD, &fds);
			if(maxfd < i->SocketFD) maxfd = i->SocketFD;
		}
		
		if( select(maxfd+1, &fds, NULL, NULL, &timeout) < 0 )
		{
			// TODO: Check error result
			return -1;
		}

		_SysDebug("select() returned");

		// Check for changes (with magic to allow inline deletion)
		for( p = (void*)&ifaces, i = ifaces; i; p=i,i = i->Next )
		{
			if( FD_ISSET(i->SocketFD, &fds) )
			{
				if( Handle_Packet( i ) )
				{
					close(i->SocketFD);
					close(i->IfaceFD);
					p->Next = i->Next;
					free(i);
					i = p;
				}
			}
			
			if( _SysTimestamp() > i->Timeout )
			{
				Handle_Timeout(i);
			}
		}
	}
	return 0;
}

void Scan_Dir(tInterface **IfaceList, const char *Directory)
{
	int dp = open(Directory, OPENFLAG_READ);
	char filename[FILENAME_MAX];

	if( dp == -1 ) {
		fprintf(stderr, "Unable to open directory '%s'\n", Directory);
	}

	while( readdir(dp, filename) )
	{
		if( filename[0] == '.' )	continue ;		
		if( strcmp(filename, "lo") == 0 )	continue ;

		tInterface *new = malloc(sizeof(tInterface) + strlen(filename)+1);
		new->Adapter = (void*)(new + 1);
		strcpy(new->Adapter, filename);
		new->Next = *IfaceList;
		*IfaceList = new;
	}
	close(dp);
}

// RETURN: Client FD
int Start_Interface(tInterface *Iface)
{
	 int	fd, tmp;
	char	path[] = "/Devices/ip/XXXXX/udpc";
	char	addr[4] = {0,0,0,0};
	
	// TODO: Check that the adapter is not in use
	
	// Request MAC address from network adapter
	{
		char	path[] = "/Devices/ip/adapters/ethXXXX";
		sprintf(path, "/Devices/ip/adapters/%s", Iface->Adapter);
		fd = open(path, 0);
		if(fd == -1) {
			_SysDebug("Unable to open adapter %s", path);
			return -1;
		}
		ioctl(fd, 4, Iface->HWAddr);
		// TODO: Check if ioctl() failed
		close(fd);
	}
	
	// Initialise an interface, with a dummy IP address (zero)
	fd = open("/Devices/ip", 0);
	if( fd == -1 ) {
		fprintf(stderr, "ERROR: Unable to open '/Devices/ip'\n"); 
		return -1;
	}
	Iface->Num = ioctl(fd, 4, (void*)Iface->Adapter);	// Create interface
	if( Iface->Num == -1 ) {
		fprintf(stderr, "ERROR: Unable to create new interface\n");
		return -1;
	}
	close(fd);
	
	// Open new interface
	snprintf(path, sizeof(path), "/Devices/ip/%i", Iface->Num);
	Iface->IfaceFD = fd = open(path, 0);
	if( fd == -1 ) {
		fprintf(stderr, "ERROR: Unable to open '%s'\n", path); 
		return -1;
	}
	tmp = 4; ioctl(fd, 4, &tmp);	// Set to IPv4
	ioctl(fd, 6, addr);	// Set address to 0.0.0.0
	tmp = 0; ioctl(fd, 7, &tmp);	// Set subnet mask to 0

	// Open UDP Client
	snprintf(path, sizeof(path), "/Devices/ip/%i/udp", Iface->Num);
	Iface->SocketFD = fd = open(path, O_RDWR);
	if( fd == -1 ) {
		fprintf(stderr, "ERROR: Unable to open '%s'\n", path); 
		return -1;
	}
	tmp = 68; ioctl(fd, 4, &tmp);	// Local port
	tmp = 67; ioctl(fd, 5, &tmp);	// Remote port
	tmp = 0;	ioctl(fd, 7, &tmp);	// Remote addr mask bits - we don't care where the reply comes from
	addr[0] = addr[1] = addr[2] = addr[3] = 255;	// 255.255.255.255
	ioctl(fd, 8, addr);	// Remote address
	
	return 0;
}

void Send_DHCPRELEASE(tInterface *Iface)
{
}

void Send_DHCPDISCOVER(tInterface *Iface)
{
	uint32_t	transaction_id;
	struct sDHCP_Message	*msg;
	char	data[8 + sizeof(struct sDHCP_Message) + 3 + 1];
	msg = (void*)data + 8;
	
	_SysDebug("DHCPDISCOVER to %s", Iface->Adapter);

	transaction_id = rand();
	Iface->TransactionID = transaction_id;

	msg->op    = htonb(1);	// BOOTREQUEST
	msg->htype = htonb(1);	// 10mb Ethernet
	msg->hlen  = htonb(6);	// 6 byte MAC
	msg->hops  = htonb(0);	// Hop count so far
	msg->xid   = htonl(transaction_id);	// Transaction ID
	msg->secs  = htons(0);	// secs - No time has elapsed
	msg->flags = htons(0x0000);	// flags - Broadcast is unset
	msg->ciaddr = htonl(0);	// ciaddr - Zero, as we don't have one yet
	msg->yiaddr = htonl(0);	// yiaddr - Zero?
	msg->siaddr = htonl(0);	// siaddr - Zero? maybe -1
	msg->giaddr = htonl(0);	// giaddr - Zero?
	memcpy(msg->chaddr, Iface->HWAddr, 6);

	memset(msg->sname, 0, sizeof(msg->sname));	// Nuke the rest
	memset(msg->file, 0, sizeof(msg->file));	// Nuke the rest
	msg->dhcp_magic = htonl(DHCP_MAGIC);

	int i = 0;
	msg->options[i++] =  53;	// DHCP Message Type
	msg->options[i++] =   1;
	msg->options[i++] =   1;	// - DHCPDISCOVER
	msg->options[i++] = 255;	// End of list
	

	data[0] = 67;	data[1] = 0;	// Port
	data[2] = 4;	data[3] = 0;	// AddrType
	data[4] = 255;	data[5] = 255;	data[6] = 255;	data[7] = 255;

	write(Iface->SocketFD, data, sizeof(data));
	Update_State(Iface, STATE_DISCOVER_SENT);
}

void Send_DHCPREQUEST(tInterface *Iface, void *OfferPacket, int TypeOffset)
{
	struct sDHCP_Message	*msg;
	 int	i;
	msg = (void*) ((char*)OfferPacket) + 8;

	// Reuses old data :)
	msg->op    = 1;
	msg->htype = 1;
	msg->hlen  = 6;
	msg->hops  = 0;
	msg->xid   = msg->xid;
	msg->secs  = htons(0);	// TODO: Maintain times
	msg->flags = htons(0);
	memcpy(msg->chaddr, Iface->HWAddr, 6);
	memset(msg->sname, 0, sizeof(msg->sname));	// Nuke the rest
	memset(msg->file, 0, sizeof(msg->file));	// Nuke the rest

	i = 0;
	msg->options[i++] = 53;	// Message type = DHCPREQUEST
	msg->options[i++] = 1;
	msg->options[i++] = 3;
	msg->options[i++] = 50;	// Requested Address
	msg->options[i++] = 4;
	memcpy(msg->options + i, &msg->yiaddr, 4);	i += 4;
//	msg->options[i++] = 54;	// Server identifier
//	msg->options[i++] = 4;
//	memcpy(msg->options + i, (char*)OfferPacket + 4, 4);	i += 4;
	msg->options[i++] = 255;
	
	// Clear last because yiaddr is needed in option setup
	msg->ciaddr = htonl(0);
	msg->yiaddr = htonl(0);
	msg->siaddr = htonl(0);
	msg->giaddr = htonl(0);

	// HACK
	((uint8_t*)OfferPacket)[4] = 255;
	((uint8_t*)OfferPacket)[5] = 255;
	((uint8_t*)OfferPacket)[6] = 255;
	((uint8_t*)OfferPacket)[7] = 255;
	
	write(Iface->SocketFD, OfferPacket, 8 + sizeof(*msg) + i);
	Update_State(Iface, STATE_REQUEST_SENT);
}

int Handle_Packet(tInterface *Iface)
{
	char	data[8+576];
	struct sDHCP_Message	*msg = (void*)(data + 8);
	 int	len, i;
	
	 int	dhcp_msg_type = 0, dhcp_msg_type_ofs;
	void	*router = NULL;
	void	*subnet_mask = NULL;
	
	_SysDebug("Doing read on %i", Iface->SocketFD);
	len = read(Iface->SocketFD, data, sizeof(data));
	_SysDebug("len = %i", len);

	_SysDebug("*msg = {");
	_SysDebug("  .op = %i", msg->op);
	_SysDebug("  .htype = %i", msg->htype);
	_SysDebug("  .ciaddr = 0x%x", ntohl(msg->ciaddr));
	_SysDebug("  .yiaddr = 0x%x", ntohl(msg->yiaddr));
	_SysDebug("}");
	if( msg->op != 2 ) {
		// Not a response
		_SysDebug("Not a response message");
		return 0;
	}

	if( htonl(msg->dhcp_magic) != DHCP_MAGIC ) {
		_SysDebug("DHCP magic doesn't match (got 0x%x, expected 0x%x)",
			htonl(msg->dhcp_magic), DHCP_MAGIC);
		return 0;
	}	


	// Check if the packet is related to our requests
	if( ntohl(msg->xid) != Iface->TransactionID ) {
		_SysDebug("Transaction ID mis-match, ignoring (0x%x != 0x%x)",
			ntohl(msg->xid), Iface->TransactionID);
		return 0;
	}
	if( memcmp(msg->chaddr, Iface->HWAddr, 6) != 0 ) {
		_SysDebug("Hardware address mis-match, ignoring");
		return 0;
	}

	// Parse options
	i = 0;
	while( i < len - sizeof(*msg) - 8 && msg->options[i] != 255 )
	{
		if( msg->options[i] == 0 ) {
			i ++;
			continue ;
		}
		_SysDebug("Option %i, %i bytes long", msg->options[i], msg->options[i+1]);
		switch(msg->options[i])
		{
		case 1:
			subnet_mask = &msg->options[i+2];
			break;
		case 3:
			router = &msg->options[i+2];
			break;
		case 53:
			dhcp_msg_type_ofs = i;
			dhcp_msg_type = msg->options[i+2];
			break;
		}
		i += msg->options[i+1]+2;
	}
	
	_SysDebug("dhcp_msg_type = %i", dhcp_msg_type);

	switch( dhcp_msg_type )
	{
	case 1:	// DHCPDISCOVER - wut?
		break;
	case 2:	// DHCPOFFER
		// Send out request for this address
		if( Iface->State != STATE_DISCOVER_SENT ) {
			_SysDebug("Ignoring DHCPOFFER when not in STATE_DISCOVER_SENT");
			return 0;
		}
		Send_DHCPREQUEST(Iface, data, dhcp_msg_type_ofs);
		break;
	case 3:	// DHCPREQUEST - wut?
		break;
	case 4:	// DHCPDECLINE - ?
		break;
	case 5:	// DHCPACK
		SetAddress(Iface, &msg->yiaddr, subnet_mask, router);
		// Return 1 to remove from list
		return 1;
	}
	return 0;
}

void Handle_Timeout(tInterface *Iface)
{
	switch(Iface->State)
	{
	case STATE_DISCOVER_SENT:
		Send_DHCPDISCOVER(Iface);
		break;
	default:
		_SysDebug("Timeout with state = %i", Iface->State);
		break;
	}
}

void Update_State(tInterface *Iface, int newState)
{
	if( Iface->State != newState )
	{
		Iface->Timeout = _SysTimestamp() + 500;
		Iface->State = newState;
	}
	else
	{
		// TODO: Exponential backoff
		Iface->Timeout = _SysTimestamp() + 3000;
		_SysDebug("State %i repeated, timeout is 3000ms now", newState);
	}
}

void SetAddress(tInterface *Iface, void *Addr, void *Mask, void *Router)
{
	 int	mask_bits = 0;	

	// Translate the mask
	if( Mask )
	{
		uint8_t	*mask = Mask;
		 int	i;
		_SysDebug("Mask %i.%i.%i.%i", mask[0], mask[1], mask[2], mask[3]);
		for( i = 0; i < 4 && mask[i] == 0xFF; i ++ ) ;
		mask_bits = i*8;
		if( i == 4 )
		{
			// Wut? /32?
		}
		else
		{
			switch(mask[i])
			{
			case 0x00:	mask_bits += 0;	break;
			case 0x80:	mask_bits += 1;	break;
			case 0xC0:	mask_bits += 2;	break;
			case 0xE0:	mask_bits += 3;	break;
			case 0xF0:	mask_bits += 4;	break;
			case 0xF8:	mask_bits += 5;	break;
			case 0xFC:	mask_bits += 6;	break;
			case 0xFE:	mask_bits += 7;	break;
			default:
				// Bad mask!
				break;
			}
		}
	}
	
	{
		uint8_t	*addr = Addr;
		_SysDebug("Addr %i.%i.%i.%i/%i", addr[0], addr[1], addr[2], addr[3], mask_bits);

		printf("Assigned %i.%i.%i.%i/%i to IF#%i (%s)\n",
			addr[0], addr[1], addr[2], addr[3], mask_bits,
			Iface->Num, Iface->Adapter
			);
	}

	ioctl(Iface->IfaceFD, 6, Addr);
	ioctl(Iface->IfaceFD, 7, &mask_bits);

	if( Router );
	{
		uint8_t	*addr = Router;
		_SysDebug("Router %i.%i.%i.%i", addr[0], addr[1], addr[2], addr[3]);
		
		// Set default route
		 int	fd;
		fd = open("/Devices/ip/routes/4:00000000:0:0", OPENFLAG_CREATE);
		if(fd == -1) {
			fprintf(stderr, "ERROR: Unable to open default route\n");
		}
		else {
			char ifname[snprintf(NULL,0,"%i",Iface->Num)+1];
			sprintf(ifname, "%i", Iface->Num);
			ioctl(fd, ioctl(fd, 3, "set_nexthop"), Router);
			ioctl(fd, ioctl(fd, 3, "set_interface"), ifname);
			close(fd);
		}
	}
}

