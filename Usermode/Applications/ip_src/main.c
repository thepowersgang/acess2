/*
 * Acess2 ip command
 * - By John Hodge (thePowersGang)
 */
#include "common.h"

// === CONSTANTS ===

// === PROTOTYPES ===
void	PrintUsage(const char *ProgName);
 int	ParseIPAddress(const char *Address, uint8_t *Dest, int *SubnetBits);

// === CODE ===
/**
 * \brief Program entrypoint
 */
int main(int argc, char *argv[])
{
	// No args, dump interfaces
	if(argc == 1) {
//		DumpInterfaces();
		return 0;
	}
	
	// Routes
	if( strcmp(argv[1], "route") == 0 )
	{
		Routes_main(argc-1, argv+1);
		return 0;
	}
	else if( strcmp(argv[1], "addr") == 0 )
	{
		Addr_main(argc-1, argv+1);
		return 0;
	}
	else if( strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 )
	{
		PrintUsage(argv[0]);
		return 0;
	}
	
	return 0;
}

/**
 * \brief Print usage instructions
 */
void PrintUsage(const char *ProgName)
{
	fprintf(stderr, "Usage:\n");
	fprintf(stderr, "    %s addr add <device> <ip>/<prefix>\n", ProgName);
	fprintf(stderr, "        Add a new interface listening on <device> with the specified\n");
	fprintf(stderr, "        address.\n");
	fprintf(stderr, "    %s addr del <interface>\n", ProgName);
	fprintf(stderr, "        Delete an interface\n");
	fprintf(stderr, "    %s addr [<interface>]\n", ProgName);
	fprintf(stderr, "        Print the current interfaces (or only <interface> if passed)\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "    %s route\n", ProgName);
	fprintf(stderr, "        Print the routing tables\n");
	fprintf(stderr, "    %s route add <host>[/<prefix>] [<nexthop> OR <iface>] [<metric>]\n", ProgName);
	fprintf(stderr, "        Add a new route\n");
	fprintf(stderr, "    %s route del <host>[/<prefix>]\n", ProgName);
	fprintf(stderr, "    %s route del <routenum>\n", ProgName);
	fprintf(stderr, "        Add a new route\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "A note on Acess's IP Stack:\n");
	fprintf(stderr, "    Each interface corresponds to only one IP address (either IPv4\n");
	fprintf(stderr, "    or IPv6). A network device can have multiple interfaces bound\n");
	fprintf(stderr, "    to it, allowing multiple addresses for one network connection\n");
	fprintf(stderr, "\n");
}


/**
 * \brief Parse an IP Address
 * \return 0 for unknown, 4 for IPv4 and 6 for IPv6
 */
int ParseIPAddress(const char *Address, uint8_t *Dest, int *SubnetBits)
{
	const char	*p = Address;
	
	// Check first block
	while(*p && *p >= '0' && *p <= '9')	p ++;
	
	// IPv4?
	if(*p == '.')
	{
		 int	i = 0, j;
		 int	val;
		
		for( j = 0; Address[i] && j < 4; j ++ )
		{
			val = 0;
			for( ; '0' <= Address[i] && Address[i] <= '9'; i++ )
			{
				val = val*10 + Address[i] - '0';
			}
			if(val > 255) {
				//printf("val > 255 (%i)\n", val);
				return 0;
			}
			Dest[j] = val;
			
			if(Address[i] == '.')
				i ++;
		}
		if( j != 4 ) {
			//printf("4 parts expected, %i found\n", j);
			return 0;
		}
		// Parse subnet size
		if(Address[i] == '/') {
			val = 0;
			i ++;
			while('0' <= Address[i] && Address[i] <= '9') {
				val *= 10;
				val += Address[i] - '0';
				i ++;
			}
			if(val > 32) {
				printf("Notice: Subnet size >32 (%i)\n", val);
			}
			if(SubnetBits)	*SubnetBits = val;
		}
		if(Address[i] != '\0') {
			//printf("EOS != '\\0', '%c'\n", Address[i]);
			return 0;
		}
		return 4;
	}
	
	// IPv6
	if(*p == ':' || ('a' <= *p && *p <= 'f') || ('A' <= *p && *p <= 'F'))
	{
		 int	i = 0;
		 int	j, k;
		 int	val, split = -1, end;
		uint16_t	hi[8], low[8];
		
		for( j = 0; Address[i] && j < 8; j ++ )
		{
			if(Address[i] == '/')
				break;
			
			if(Address[i] == ':') {
				if(split != -1) {
					printf("Two '::'s\n");
					return 0;
				}
				split = j;
				i ++;
				continue;
			}
			
			val = 0;
			for( k = 0; Address[i] && Address[i] != ':' && Address[i] != '/'; i++, k++ )
			{
				val *= 16;
				if('0' <= Address[i] && Address[i] <= '9')
					val += Address[i] - '0';
				else if('A' <= Address[i] && Address[i] <= 'F')
					val += Address[i] - 'A' + 10;
				else if('a' <= Address[i] && Address[i] <= 'f')
					val += Address[i] - 'a' + 10;
				else {
					printf("%c unexpected\n", Address[i]);
					return 0;
				}
			}
			
			if(val > 0xFFFF) {
				printf("val (0x%x) > 0xFFFF\n", val);
				return 0;
			}
			
			if(split == -1)
				hi[j] = val;
			else
				low[j-split] = val;
			
			if( Address[i] == ':' ) {
				i ++;
			}
		}
		end = j;
		
		// Parse subnet size
		if(Address[i] == '/') {
			val = 0;
			while('0' <= Address[i] && Address[i] <= '9') {
				val *= 10;
				val += Address[i] - '0';
				i ++;
			}
			if(val > 128) {
				printf("Notice: Subnet size >128 (%i)\n", val);
			}
			if(SubnetBits)	*SubnetBits = val;
		}
		
		for( j = 0; j < split; j ++ )
		{
			//printf("%04x:", hi[j]);
			Dest[j*2] = hi[j]>>8;
			Dest[j*2+1] = hi[j]&0xFF;
		}
		for( ; j < 8 - (end - split); j++ )
		{
			//printf("0000:", hi[j]);
			Dest[j*2] = 0;
			Dest[j*2+1] = 0;
		}
		for( k = 0; j < 8; j ++, k++)
		{
			//printf("%04x:", low[k]);
			Dest[j*2] = low[k]>>8;
			Dest[j*2+1] = low[k]&0xFF;
		}
		return 6;
	}
	// Unknown type
	return 0;
}
