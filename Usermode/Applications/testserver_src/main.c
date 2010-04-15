/*
 * Acess2 Test Server
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
	 int	srv = -1;
	 int	con = -1;
	 int	len;
	uint16_t	port;
	char	buf[8+1];
	uint8_t	data[4096];	// Packet Data
	 
	srv = open("/Devices/ip/1/tcps", OPENFLAG_READ|OPENFLAG_EXEC);
	if(srv == -1) {
		fprintf(stderr, "Unable top open TCP server '/Devices/ip/1/tcps'\n");
		return -1;
	}
	port = 80;	ioctl(srv, 4, &port);	// Set Port
	
	for(;;)
	{
		readdir( srv, buf );
		printf("Connection '/Devices/ip/1/tcps/%s'\n", buf);
		con = _SysOpenChild(srv, buf, OPENFLAG_READ|OPENFLAG_WRITE);
		if(con == -1) {
			fprintf(stderr, "Wtf! Why didn't this open?\n");
			return 1;
		}
		
		len = read(con, 4096, data);
		if( len == -1 ) {
			printf("Connection closed\n");
			close(con);
			continue;
		}
		if( len != 0 )
		{
			printf("Packet Data: (%i bytes)\n", len);
			printf("%s\n", data);
			printf("--- EOP ---\n");
		}
		
		#define RET_STR	"HTTP/1.1 200 OK\r\n"\
			"Content-Type: text/plain\r\n"\
			"Content-Length: 6\r\n"\
			"\r\n"\
			"Hello!"
		
		write(con, sizeof(RET_STR), RET_STR);
		
		close(con);
	}
	
	return 0;
}
