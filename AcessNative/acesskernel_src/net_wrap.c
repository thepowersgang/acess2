/*
 * Acess2 Native Kernel
 * - Acess kernel emulation on another OS using SDL and UDP
 *
 * net.c
 * - Networking
 */
#define DEBUG	1
#include <stdlib.h>
#include <unistd.h>
#include "../../KernelLand/Kernel/include/logdebug.h"
#include "net_wrap.h"
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/eventfd.h>
#include <poll.h>

#include <SDL/SDL.h>	// for worker

typedef struct sNetSocket	tNetSocket;

struct sNetSocket
{
	tNetSocket	*Next;
	tNetSocket	*Prev;
	void	*Node;
	 int	FD;
	char	bSkipRead;
	char	bSkipWrite;
	char	bSkipError;
};


 int	Net_Wrap_WatcherThread(void *arg);

 int	gNet_Wrap_EventFD;
tNetSocket	*gNet_Wrap_FirstSocket;
tNetSocket	*gNet_Wrap_LastSocket;
SDL_Thread	*gNet_Wrap_SelectChecker;

// === CODE ===
void Net_Wrap_Init(void)
{
	gNet_Wrap_EventFD = eventfd(0, 0);
	gNet_Wrap_SelectChecker = SDL_CreateThread( Net_Wrap_WatcherThread, NULL );
}

static inline void Net_Wrap_ProdWatcher(void)
{
	uint64_t val = 1;
	write(gNet_Wrap_EventFD, &val, sizeof(val));
}

int Net_Wrap_WatcherThread(void *arg)
{
	for(;;)
	{
		 int	maxfd = gNet_Wrap_EventFD;
		fd_set	rfds, wfds, efds;
		FD_ZERO(&rfds);
		FD_ZERO(&wfds);
		FD_ZERO(&efds);
		// Event FD used to wake this thread when the list changes
		FD_SET(gNet_Wrap_EventFD, &rfds);
		// Populate socket FDs
		for( tNetSocket *s = gNet_Wrap_FirstSocket; s; s = s->Next )
		{
			if( s->FD > maxfd )
				maxfd = s->FD;
			if( !s->bSkipRead )	FD_SET(s->FD, &rfds);
			if( !s->bSkipWrite )	FD_SET(s->FD, &wfds);
			if( !s->bSkipError )	FD_SET(s->FD, &efds);
		}
	
		LOG("select(%i+1, ...)", maxfd);
		int rv = select(maxfd+1, &rfds, &wfds, &efds, NULL);
		LOG("rv = %i");
		if( rv <= 0 )
			continue ;

		if( FD_ISSET(gNet_Wrap_EventFD, &rfds) )
		{
			uint64_t	val;
			read(gNet_Wrap_EventFD, &val, 8);
		}	

		for( tNetSocket *s = gNet_Wrap_FirstSocket; s; s = s->Next )
		{
			if( FD_ISSET(s->FD, &rfds) ) {
				VFS_MarkAvaliable(s->Node, 1);
				s->bSkipRead = 1;
			}
			if( FD_ISSET(s->FD, &wfds) ) {
				VFS_MarkFull(s->Node, 0);
				s->bSkipWrite = 1;
			}
			if( FD_ISSET(s->FD, &efds) ) {
				VFS_MarkError(s->Node, 1);
				s->bSkipError = 1;	// TODO: how will err be acked?
			}
		}
	}
}

void *Net_Wrap_ConnectTcp(void *Node, short SrcPort, short DstPort, int Type, const void *Addr)
{
	struct sockaddr_in	addr4;
	struct sockaddr	*addr;
	socklen_t	addr_size;
	
	 int	af;
	switch(Type)
	{
	case 4:
		af = AF_INET;
		addr = (struct sockaddr*)&addr4;
		addr_size = sizeof(addr4);
		memset(addr, 0, addr_size);
		LOG("%02x%02x%02x%02x",
			((uint8_t*)Addr)[0], ((uint8_t*)Addr)[1],
			((uint8_t*)Addr)[2], ((uint8_t*)Addr)[3]);
		memcpy(&addr4.sin_addr, Addr, 4);
		//addr4.sin_addr.s_addr = htonl( addr4.sin_addr.s_addr);
		addr4.sin_port = htons(DstPort);
		break;
	//case 6:	af = AF_INET6;	break;
	default:
		return NULL;
	}	
	
	addr->sa_family = af;

	int fd = socket(af, SOCK_STREAM, 0);
	if( SrcPort ) {
		Log_Warning("NetWrap", "TODO: Support setting TCP source port");
	}
	Debug_HexDump("NetWrap: addr=", addr, addr_size);
	if( connect(fd, addr, addr_size) ) {
		close(fd);
		Log_Notice("NetWrap", "connect() failed: %s", strerror(errno));
		char tmp[64] = {0};
		inet_ntop(af, addr->sa_data+2, tmp, sizeof(tmp));
		printf("addr = %p %s\n", tmp, tmp);
		return NULL;
	}
	
	tNetSocket	*ret = malloc( sizeof(tNetSocket) );
	if(!ret) {
		close(fd);
		return NULL;
	}
	
	ret->Next = NULL;
	ret->Node = Node;
	ret->FD = fd;
	ret->bSkipRead = 0;
	ret->bSkipWrite = 0;
	ret->bSkipError = 0;

	if( gNet_Wrap_FirstSocket ) {
		gNet_Wrap_LastSocket->Next = ret;
		ret->Prev = gNet_Wrap_LastSocket;
	}
	else {
		gNet_Wrap_FirstSocket = ret;
		ret->Prev = NULL;
	}
	gNet_Wrap_LastSocket = ret;

	Net_Wrap_ProdWatcher();	

	return ret;
}

static void Net_Wrap_UpdateFlags(tNetSocket *hdl)
{
	struct pollfd fdinfo = {.fd = hdl->FD, .events=POLLIN|POLLOUT, .revents=0};
	
	poll(&fdinfo, 1, 0);
	
	VFS_MarkAvaliable(hdl->Node, !!(fdinfo.revents & POLLIN));
	VFS_MarkFull(hdl->Node, !(fdinfo.revents & POLLOUT));

	// If no data can be written, re-enable select	
	if( !(fdinfo.revents & POLLOUT) )
		hdl->bSkipWrite = 0;
	// Same for if no data to read
	if( !(fdinfo.revents & POLLIN) )
		hdl->bSkipRead = 0;
	
	Net_Wrap_ProdWatcher();	
}

size_t Net_Wrap_ReadSocket(void *Handle, size_t Bytes, void *Dest)
{
	tNetSocket	*hdl = Handle;
	if(!hdl)	return -1;
	size_t rv = read(hdl->FD, Dest, Bytes);
	Net_Wrap_UpdateFlags(hdl);	
	return rv;
}

size_t Net_Wrap_WriteSocket(void *Handle, size_t Bytes, const void *Dest)
{
	tNetSocket	*hdl = Handle;
	if(!hdl)	return -1;
	size_t rv = write(hdl->FD, Dest, Bytes);
	Net_Wrap_UpdateFlags(hdl);	
	return rv;
}

void Net_Wrap_CloseSocket(void *Handle)
{
	tNetSocket	*hdl = Handle;

	// TODO: Lock
	if(hdl->Prev)
		hdl->Prev->Next = hdl->Next;
	else 
		gNet_Wrap_FirstSocket = hdl->Next;
	if(hdl->Next)
		hdl->Next->Prev = hdl->Prev;
	else
		gNet_Wrap_LastSocket = hdl->Prev;	
	Net_Wrap_ProdWatcher();	

	close(hdl->FD);
	free(hdl);
}

