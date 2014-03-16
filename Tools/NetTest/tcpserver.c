/*
 * Acess2 Networking Test Suite (NetTest)
 * - By John Hodge (thePowersGang)
 *
 * tcpserver.c
 * - TCP Client tester
 */
#include <vfs.h>
#include <vfs_ext.h>
#include <nettest.h>
#include "tcpserver.h"

#define MAX_CLIENTS	4

struct sNetTest_TCPServer
{
	 int	ServerFD;
	 int	nClients;
	struct sClient {
		 int	FD;
	} Clients[MAX_CLIENTS];
};

// === CODE ===
tNetTest_TCPServer *NetTest_TCPServer_Create(int Port)
{
	tNetTest_TCPServer *ret;
	ret = calloc(sizeof(*ret), 1);
	ret->ServerFD = VFS_Open("/Devices/ip/1/tcps", VFS_OPENFLAG_READ);
	ASSERT(ret->ServerFD >= 0);
	VFS_IOCtl(ret->ServerFD, 4, &Port);
	return ret;
}

void NetTest_TCPServer_Close(tNetTest_TCPServer *Srv)
{
	VFS_Close(Srv->ServerFD);
	for( int i = 0; i < Srv->nClients; i ++ )
	{
		struct sClient	*client = &Srv->Clients[i];
		VFS_Close(client->FD);
	}
	free(Srv);
}

int NetTest_TCPServer_FillSelect(tNetTest_TCPServer *Srv, fd_set *fds)
{
	ASSERT(Srv->ServerFD >= 0);
	 int	max = -1;
	
	if( Srv->nClients == MAX_CLIENTS ) {
		max = Srv->ServerFD;
		FD_SET(Srv->ServerFD, fds);
	}
	for( int i = 0; i < Srv->nClients; i ++ )
	{
		 int	fd = Srv->Clients[i].FD;
		if( fd > max )	max = fd;
		FD_SET(fd, fds);
	}
	return max;
}

void NetTest_TCPServer_HandleSelect(tNetTest_TCPServer *Srv, const fd_set *rfds, const fd_set *wfds, const fd_set *efds)
{
	if( FD_ISSET(Srv->ServerFD, rfds) )
	{
		// New connection!
		ASSERT(Srv->nClients != MAX_CLIENTS);
		struct sClient *client = &Srv->Clients[Srv->nClients++];
		client->FD = VFS_OpenChild(Srv->ServerFD, "", VFS_OPENFLAG_READ|VFS_OPENFLAG_WRITE);
	}

	for( int i = 0; i < Srv->nClients; i ++ )
	{
		struct sClient	*client = &Srv->Clients[i];
		if( FD_ISSET(client->FD, rfds) )
		{
			// RX'd data on client
		}
		
		if( FD_ISSET(client->FD, efds) )
		{
			// Drop client
			VFS_Close(client->FD);
			memmove(client, client+1, (Srv->nClients-i-1) * sizeof(*client));
			Srv->nClients --;
			i --;	// counteract i++
		}
	}
}

