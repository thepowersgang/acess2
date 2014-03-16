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
#include <sys/socket.h>
#include <sys/un.h>

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

void *NetTest_OpenUnix(const char *Path)
{
	int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	struct sockaddr_un	sa = {AF_UNIX, ""};
	struct sockaddr_un	sa_local = {AF_UNIX, ""};
	strcpy(sa.sun_path, Path);
	if( connect(fd, (struct sockaddr*)&sa, sizeof(sa)) ) {
		perror("NetTest_OpenUnix - connect");
		close(fd);
		return NULL;
	}
	if( bind(fd, (struct sockaddr*)&sa_local, sizeof(sa)) ) {
		perror("NetTest_OpenUnix - bind");
		close(fd);
		return NULL;
	}
	
	{
		char somenulls[] = { 0,0,0,0,0,0, 0,0,0,0,0, 0,0};
		write(fd, somenulls, sizeof(somenulls));
	}
	
	return (void*)(intptr_t)fd;
}

size_t NetTest_WritePacket(void *Handle, size_t Size, const void *Data)
{
	int ret = write( (intptr_t)Handle, Data, Size);
	if( ret < 0 ) {
		perror("NetTest_WritePacket - write");
		return 0;
	}
	return ret;
}

size_t NetTest_ReadPacket(void *Handle, size_t MaxSize, void *Data)
{
	return read( (intptr_t)Handle, Data, MaxSize);
}
