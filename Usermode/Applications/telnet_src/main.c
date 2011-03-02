/*
 AcessOS Test Network Application
*/
#include <acess/sys.h>
#include <stdio.h>
#include <net.h>

#define BUFSIZ	128

// === PROTOTYPES ===
 int	main(int argc, char *argv[], char *envp[]);
 int	OpenTCP(const char *AddressString, short PortNumber);

// ==== CODE ====
int main(int argc, char *argv[], char *envp[])
{
	 int	server_fd, rv;
	 int	client_running = 1;
	
	// Connect to the remove server
	server_fd = OpenTCP( argv[1], atoi(argv[2]) );
	if( server_fd == -1 ) {
		fprintf(stderr, "Unable to create socket\n");
		return -1;
	}
	
	while( client_running )
	{
		fd_set	fds;
		fd_set	err_fds;
		char	buffer[BUFSIZ];
		 int	len;
		
		FD_ZERO(&fds);	FD_ZERO(&err_fds);
		FD_SET(0, &fds);	FD_SET(0, &err_fds);
		FD_SET(server_fd, &fds);	FD_SET(server_fd, &err_fds);
		
		rv = select(server_fd+1, &fds, NULL, &err_fds, NULL);
		if( rv < 0 )	break;
		
		if( FD_ISSET(server_fd, &fds) )
		{
			// Read from server, and write to stdout
			do
			{
				len = read(server_fd, BUFSIZ, buffer);
				write(1, len, buffer);
			} while( len == BUFSIZ );
		}
		
		if( FD_ISSET(0, &fds) )
		{
			// Read from stdin, and write to server
			do
			{
				len = read(0, BUFSIZ, buffer);
				write(server_fd, len, buffer);
				write(1, len, buffer);
			} while( len == BUFSIZ );
		}
		
		if( FD_ISSET(server_fd, &err_fds) )
		{
			break ;
		}
	}
	
	close(server_fd);
	return 0;
}

/**
 * \brief Initialise a TCP connection to \a AddressString on port \a PortNumber
 */
int OpenTCP(const char *AddressString, short PortNumber)
{
	 int	fd, addrType;
	char	*iface;
	char	addrBuffer[8];
	
	// Parse IP Address
	addrType = Net_ParseAddress(AddressString, addrBuffer);
	if( addrType == 0 ) {
		fprintf(stderr, "Unable to parse '%s' as an IP address\n", AddressString);
		return -1;
	}
	
	// Finds the interface for the destination address
	iface = Net_GetInterface(addrType, addrBuffer);
	if( iface == NULL ) {
		fprintf(stderr, "Unable to find a route to '%s'\n", AddressString);
		return -1;
	}
	
	printf("iface = '%s'\n", iface);
	
	// Open client socket
	// TODO: Move this out to libnet?
	{
		 int	len = snprintf(NULL, 100, "/Devices/ip/%s/tcpc", iface);
		char	path[len+1];
		snprintf(path, 100, "/Devices/ip/%s/tcpc", iface);
		fd = open(path, OPENFLAG_READ|OPENFLAG_WRITE);
	}
	
	free(iface);
	
	if( fd == -1 ) {
		fprintf(stderr, "Unable to open TCP Client for reading\n");
		return -1;
	}
	
	// Set remote port and address
	printf("Setting port and remote address\n");
	ioctl(fd, 5, &PortNumber);
	ioctl(fd, 6, addrBuffer);
	
	// Connect
	printf("Initiating connection\n");
	if( ioctl(fd, 7, NULL) == 0 ) {
		// Shouldn't happen :(
		fprintf(stderr, "Unable to start connection\n");
		return -1;
	}
	
	// Return descriptor
	return fd;
}

