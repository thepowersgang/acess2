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


#define CONNECT_TIMEOUT	10*1000
#define MAX_IFS	4

typedef struct {
	int	FD;
	socklen_t	addrlen;
	struct sockaddr_un	addr;
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

size_t Net_Receive(int IfNum, size_t MaxLen, void *DestBuf, unsigned int Timeout)
{
	assert(IfNum < MAX_IFS);
	tIf	*If = &gaInterfaces[IfNum];
	
	if( Net_int_EnsureConnected(IfNum) && WaitOnFD(If->FD, false, Timeout) )
	{
		return recvfrom(If->FD, DestBuf, MaxLen, 0, &If->addr, &If->addrlen);
	}
	return 0;
}

void Net_Send(int IfNum, size_t Length, const void *Buf)
{
	assert(IfNum < MAX_IFS);
	tIf	*If = &gaInterfaces[IfNum];
	
	if( !WaitOnFD(If->FD, true, CONNECT_TIMEOUT) )
		return ;
	int rv = sendto(If->FD, Buf, Length, 0, &If->addr, If->addrlen);
	if( rv < 0 )
		perror("Net_Send - send");
}


