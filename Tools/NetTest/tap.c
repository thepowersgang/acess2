/*
 * Acess2 Networking Test Suite (NetTest)
 * - By John Hodge (thePowersGang)
 *
 * tap.c
 * - TAP Network driver
 */
#include <nettest.h>
#include <sys/ioctl.h>
#include <net/if.h>
//#include <linux/if.h>
#include <linux/if_tun.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

// === CODE ===
void *NetTest_OpenTap(const char *Name)
{
	 int	rv;

	struct ifreq	ifr;
	memset(&ifr, 0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP|IFF_NO_PI;
	
	if( Name[0] != '\0' )
	{
		if( strlen(Name) > IFNAMSIZ )
			return NULL;
		strncpy(ifr.ifr_name, Name, IFNAMSIZ);
	}
	
	 int	fd = open("/dev/net/tun", O_RDWR);
	if( fd < 0 )
	{
		perror("NetTest_OpenTap - open");
		return NULL;
	}
	
	if( (rv = ioctl(fd, TUNSETIFF, &ifr)) )
	{
		perror("NetTest_OpenTap - ioctl(TUNSETIFF)");
		fprintf(stderr, "Opening TUN/TAP device '%s'\n", Name);
		close(fd);
		return NULL;
	}

	return (void*)(intptr_t)fd;
}

size_t NetTest_WritePacket(void *Handle, size_t Size, const void *Data)
{
	return write( (intptr_t)Handle, Data, Size);
}

size_t NetTest_ReadPacket(void *Handle, size_t MaxSize, void *Data)
{
	return read( (intptr_t)Handle, Data, MaxSize);
}
