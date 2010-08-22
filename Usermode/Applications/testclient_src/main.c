/*
 * Acess2 Test Client
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	 int	con = -1;
	 int	len;
	uint16_t	port;
	uint8_t	buf[4] = {10,0,2,1};
	uint8_t	data[4096];	// Packet Data
	 
	con = open("/Devices/ip/1/tcpc", OPENFLAG_READ|OPENFLAG_WRITE);
	if(con == -1) {
		fprintf(stderr, "Unable top open TCP client '/Devices/ip/1/tcpc'\n");
		return -1;
	}
	
	port = 80;	ioctl(con, 5, &port);	// Set Remote Port
	ioctl(con, 6, buf);	// Set remote IP
	ioctl(con, 7, NULL);	// Connect
	
	#define REQ_STR	"GET / HTTP/1.1\r\n"\
		"Host: prelude\r\n"\
		"User-Agent: Acess2 TCP Test Client\r\n"\
		"\r\n"
	
	write(con, sizeof(REQ_STR)-1, REQ_STR);
	
	len = read(con, 4096, data);
	close(con);
	if( len == -1 ) {
		printf("Connection closed on us\n");
		return 0;
	}
	if( len != 0 )
	{
		printf("Packet Data: (%i bytes)\n", len);
		printf("%s\n", data);
		printf("--- EOP ---\n");
	}
	return 0;
}
