/*
 */
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>
#include <sys/select.h>
#include "net.h"
#include <stdint.h>
#include <unistd.h>	// unlink/...
#include <sys/time.h>	// gettimeofday

#define CONNECT_TIMEOUT	10*1000
#define MAX_IFS	4

typedef struct {
	 int	FD;
	socklen_t	addrlen;
	struct sockaddr_un	addr;
	FILE	*CapFP;
} tIf;

// === PROTOTYPES ===
 int	Net_int_Open(const char *Path);

// === GLOBALS ===
tIf	gaInterfaces[MAX_IFS];

// === CODE ===
int Net_Open(int IfNum, const char *Path)
{
	if( IfNum >= MAX_IFS )
		return 1;
	
	if(gaInterfaces[IfNum].FD != 0)	return 1;
	gaInterfaces[IfNum].addrlen = sizeof(gaInterfaces[IfNum].addr);
	gaInterfaces[IfNum].FD = Net_int_Open(Path);
	
	char cappath[] = "testif00.pcap";
	sprintf(cappath, "testif%i.pcap", IfNum);
	gaInterfaces[IfNum].CapFP = fopen(cappath, "w");
	{
		struct {
			uint32_t magic_number;   /* magic number */
			uint16_t version_major;  /* major version number */
			uint16_t version_minor;  /* minor version number */
			 int32_t  thiszone;       /* GMT to local correction */
			uint32_t sigfigs;        /* accuracy of timestamps */
			uint32_t snaplen;        /* max length of captured packets, in octets */
			uint32_t network;        /* data link type */
		} __attribute__((packed)) hdr = {
			0xa1b2c3d4,
			2,4,
			0,
			0,
			65535,
			1
		};
		fwrite(&hdr, sizeof(hdr), 1, gaInterfaces[IfNum].CapFP);
	}
	return 0;
}

int Net_int_Open(const char *Path)
{
	int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	struct sockaddr_un	sa = {AF_UNIX, ""};
	strcpy(sa.sun_path, Path);
	unlink(Path);
	if( bind(fd, (struct sockaddr*)&sa, sizeof(sa)) ) {
		perror("NetTest_OpenUnix - bind");
		close(fd);
		return -1;
	}
	
	return fd;
}

void Net_Close(int IfNum)
{
	assert(IfNum < MAX_IFS);
	close(gaInterfaces[IfNum].FD);
	gaInterfaces[IfNum].FD = 0;
	fclose(gaInterfaces[IfNum].CapFP);
}

bool WaitOnFD(int FD, bool Write, unsigned int Timeout)
{
	fd_set	fds;
	FD_ZERO(&fds);
	FD_SET(FD, &fds);
	struct timeval	timeout;
	timeout.tv_sec = Timeout/1000;
	timeout.tv_usec = (Timeout%1000) * 1000;
	
	if( Write )
		select(FD+1, NULL, &fds, NULL, &timeout);
	else
		select(FD+1, &fds, NULL, NULL, &timeout);
	return FD_ISSET(FD, &fds);
}

bool Net_int_EnsureConnected(int IfNum)
{
	assert(IfNum < MAX_IFS);
	#if 0
	if( gaInterface_Clients[IfNum] == 0 )
	{
		//if( !WaitOnFD(gaInterface_Servers[IfNum], CONNECT_TIMEOUT) )
		//{
		//	fprintf(stderr, "ERROR: Client has not connected");
		//	return false;
		//}
		gaInterface_Clients[IfNum] = accept(gaInterface_Servers[IfNum], NULL, NULL);
		if( gaInterface_Clients[IfNum] < 0 ) {
			perror("Net_int_EnsureConnected - accept");
			return false;
		}
	}
	#endif
	return true;
}

void Net_int_SavePacket(tIf *If, size_t size, const void *data)
{
	struct timeval	curtime;
	gettimeofday(&curtime, NULL);
	struct {
		uint32_t ts_sec;
		uint32_t ts_usec;
		uint32_t incl_len;
		uint32_t orig_len;
	} __attribute__((packed)) hdr = {
		curtime.tv_sec, curtime.tv_usec,
		size, size
	};
	fwrite(&hdr, sizeof(hdr), 1, If->CapFP);
	fwrite(data, size, 1, If->CapFP);
}

size_t Net_Receive(int IfNum, size_t MaxLen, void *DestBuf, unsigned int Timeout)
{
	assert(IfNum < MAX_IFS);
	tIf	*If = &gaInterfaces[IfNum];
	
	if( Net_int_EnsureConnected(IfNum) && WaitOnFD(If->FD, false, Timeout) )
	{
		size_t rv = recvfrom(If->FD, DestBuf, MaxLen, 0, (struct sockaddr*)&If->addr, &If->addrlen);
		Net_int_SavePacket(If, rv, DestBuf);
		return rv;
	}
	return 0;
}

void Net_Send(int IfNum, size_t Length, const void *Buf)
{
	assert(IfNum < MAX_IFS);
	tIf	*If = &gaInterfaces[IfNum];
	
	if( !WaitOnFD(If->FD, true, CONNECT_TIMEOUT) )
		return ;
	Net_int_SavePacket(If, Length, Buf);
	int rv = sendto(If->FD, Buf, Length, 0, (struct sockaddr*)&If->addr, If->addrlen);
	if( rv < 0 )
		perror("Net_Send - send");
}


