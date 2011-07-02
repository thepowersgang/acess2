/*
 * Acess2 Test Client
 */
#include <acess/sys.h>
#include <stdlib.h>
#include <stdio.h>
#include <net.h>

 int	OpenTCP(const char *AddressString, short PortNumber);

/**
 * \fn int main(int argc, char *argv[])
 * \brief Entrypoint
 */
int main(int argc, char *argv[])
{
	 int	con = -1;
	 int	len;
	uint8_t	data[4096];	// Packet Data
	
	con = OpenTCP("10.0.2.2", 80);
	if(con == -1) {
		fprintf(stderr, "Unable to open TCP client\n");
		return -1;
	}
	
	#define REQ_STR	"GET / HTTP/1.1\r\n"\
		"Host: sonata\r\n"\
		"User-Agent: Acess2 TCP Test Client\r\n"\
		"\r\n"
	
	write(con, sizeof(REQ_STR)-1, REQ_STR);
	
	while( (len = read(con, 4095, data)) > 0 )
	{
		data[len] = 0;
		_SysDebug("%i bytes - %s", len, data);
		printf("%s", data);
	}
	close(con);
	return 0;
}

/**
 * \brief Initialise a TCP connection to \a AddressString on port \a PortNumber
 */
int OpenTCP(const char *AddressString, short PortNumber)
{
	 int	fd, addrType;
	uint8_t	addrBuffer[16];
	
	// Parse IP Address
	addrType = Net_ParseAddress(AddressString, addrBuffer);
	if( addrType == 0 ) {
		fprintf(stderr, "Unable to parse '%s' as an IP address\n", AddressString);
		return -1;
	}

	// Opens a R/W handle
	fd = Net_OpenSocket(addrType, addrBuffer, "tcpc");
	if( fd == -1 )
	{
		fprintf(stderr, "Unable to open TCP Client\n");
		return -1;
	}
	
	// Set remote port and address
	ioctl(fd, 5, &PortNumber);
	ioctl(fd, 6, addrBuffer);
	
	// Connect
	if( ioctl(fd, 7, NULL) == 0 ) {
		fprintf(stderr, "Unable to start connection\n");
		return -1;
	}
	
	printf("Connection opened\n");
	
	// Return descriptor
	return fd;
}

