/*
 * Acess2 GUI (AxWin) Version 3
 * - By John Hodge (thePowersGang)
 * 
 * ipc_acess.c
 * - Interprocess communication (acess handlers)
 */
#include <ipc_int.h>
#include <common.h>
#include <acess/sys.h>
#include <net.h>
#include <string.h>

// === CONSTANTS ===
#define STATICBUF_SIZE	64
#define AXWIN_PORT	4101

// === PROTOTYPES ===
void	IPC_Init(void);
void	IPC_FillSelect(int *nfds, fd_set *set, fd_set *err_set);
void	IPC_HandleSelect(fd_set *set, fd_set *err_set);
 int	IPC_Type_Datagram_GetSize(const void *Ident);
 int	IPC_Type_Datagram_Compare(const void *Ident1, const void *Ident2);
void	IPC_Type_Datagram_Send(const void *Ident, size_t Length, const void *Data);
 int	IPC_Type_Sys_GetSize(const void *Ident);
 int	IPC_Type_Sys_Compare(const void *Ident1, const void *Ident2);
void	IPC_Type_Sys_Send(const void *Ident, size_t Length, const void *Data);
 int	IPC_Type_IPCPipe_GetSize(const void *Ident);
 int	IPC_Type_IPCPipe_Compare(const void *Ident1, const void *Ident2);
void	IPC_Type_IPCPipe_Send(const void *Ident, size_t Length, const void *Data);

// === GLOBALS ===
const tIPC_Type	gIPC_Type_Datagram = {
	IPC_Type_Datagram_GetSize,
	IPC_Type_Datagram_Compare, 
	IPC_Type_Datagram_Send
};
const tIPC_Type	gIPC_Type_SysMessage = {
	IPC_Type_Sys_GetSize,
	IPC_Type_Sys_Compare,
	IPC_Type_Sys_Send
};
const tIPC_Type	gIPC_Type_IPCPipe = {
	IPC_Type_IPCPipe_GetSize,
	IPC_Type_IPCPipe_Compare,
	IPC_Type_IPCPipe_Send
};
 int	giNetworkFileHandle = -1;
 int	giIPCPipeHandle = -1;

// === CODE ===
void IPC_Init(void)
{
	 int	tmp;
	// TODO: Check this
	giNetworkFileHandle = _SysOpen("/Devices/ip/loop/udp", OPENFLAG_READ);
	if( giNetworkFileHandle != -1 )
	{
		tmp = AXWIN_PORT;
		_SysIOCtl(giNetworkFileHandle, 4, &tmp);	// TODO: Don't hard-code IOCtl number
	}
	
	giIPCPipeHandle = _SysOpen("/Devices/ipcpipe/axwin"/*-$USER*/, OPENFLAG_CREATE);
	_SysDebug("giIPCPipeHandle = %i", giIPCPipeHandle);
	if( giIPCPipeHandle == -1 )
		_SysDebug("ERROR: Can't create IPCPipe handle");
}

void _setfd(int fd, int *nfds, fd_set *set)
{
	if( fd >= 0 )
	{
		if( fd >= *nfds )	*nfds = fd+1;
		FD_SET(fd, set);
	}
}

void IPC_FillSelect(int *nfds, fd_set *set, fd_set *err_set)
{
	_setfd(giNetworkFileHandle, nfds, set);
	_setfd(giIPCPipeHandle, nfds, set);
	for( int i = 0; i < giIPC_ClientCount; i ++ )
	{
		if( gIPC_Clients[i] && gIPC_Clients[i]->IPCType == &gIPC_Type_IPCPipe ) {
			_setfd( *(int*)(gIPC_Clients[i]->Ident), nfds, set );
			_setfd( *(int*)(gIPC_Clients[i]->Ident), nfds, err_set );
		}
	}
}

void IPC_HandleSelect(fd_set *set, fd_set *err_set)
{
	if( giNetworkFileHandle != -1 && FD_ISSET(giNetworkFileHandle, set) )
	{
		char	staticBuf[STATICBUF_SIZE];
		 int	readlen, identlen;
		char	*msg;

		readlen = _SysRead(giNetworkFileHandle, staticBuf, sizeof(staticBuf));
		
		identlen = 4 + Net_GetAddressSize( ((uint16_t*)staticBuf)[1] );
		msg = staticBuf + identlen;

		IPC_Handle( IPC_int_GetClient(&gIPC_Type_Datagram, staticBuf), readlen - identlen, (void*)msg);
		//_SysDebug("IPC_HandleSelect: UDP handled");
	}
	
	if( giIPCPipeHandle != -1 && FD_ISSET(giIPCPipeHandle, set) )
	{
		int newfd = _SysOpenChild(giIPCPipeHandle, "newclient", OPENFLAG_READ|OPENFLAG_WRITE);
		_SysDebug("newfd = %i", newfd);
		IPC_int_GetClient(&gIPC_Type_IPCPipe, &newfd);
	}
	
	for( int i = 0; i < giIPC_ClientCount; i ++ )
	{
		if( gIPC_Clients[i] && gIPC_Clients[i]->IPCType == &gIPC_Type_IPCPipe )
		{
			 int fd = *(const int*)gIPC_Clients[i]->Ident;
			if( FD_ISSET(fd, set) )
			{
				char	staticBuf[STATICBUF_SIZE];
				size_t	len;
				len = _SysRead(fd, staticBuf, sizeof(staticBuf));
				if( len == (size_t)-1 ) {
					// TODO: Check errno for EINTR
					IPC_int_DropClient(gIPC_Clients[i]);
					break;
				}
				IPC_Handle( gIPC_Clients[i], len, (void*)staticBuf );
			}
			if( FD_ISSET(fd, err_set) )
			{
				// Client disconnected
				IPC_int_DropClient(gIPC_Clients[i]);
				_SysClose(fd);
			}
		}
	}

	size_t	len;
	int	tid;
	while( (len = _SysGetMessage(&tid, 0, NULL)) )
	{
		char	data[len];
		_SysGetMessage(NULL, len, data);

		IPC_Handle( IPC_int_GetClient(&gIPC_Type_SysMessage, &tid), len, (void*)data );
//		_SysDebug("IPC_HandleSelect: Message handled");
	}
}

int IPC_Type_Datagram_GetSize(const void *Ident)
{
	return 4 + Net_GetAddressSize( ((const uint16_t*)Ident)[1] );
}

int IPC_Type_Datagram_Compare(const void *Ident1, const void *Ident2)
{
	// Pass the buck :)
	// - No need to worry about mis-matching sizes, as the size is computed
	//   from the 3rd/4th bytes, hence it will differ before the size is hit.
	return memcmp(Ident1, Ident2, IPC_Type_Datagram_GetSize(Ident1));
}

void IPC_Type_Datagram_Send(const void *Ident, size_t Length, const void *Data)
{
	 int	identlen = IPC_Type_Datagram_GetSize(Ident);
	char	tmpbuf[ identlen + Length ];
	memcpy(tmpbuf, Ident, identlen);	// Header
	memcpy(tmpbuf + identlen, Data, Length);	// Data
	// TODO: Handle fragmented packets
	_SysWrite(giNetworkFileHandle, tmpbuf, sizeof(tmpbuf));
}

int IPC_Type_Sys_GetSize(const void *Ident)
{
	return sizeof(pid_t);
}

int IPC_Type_Sys_Compare(const void *Ident1, const void *Ident2)
{
	return *(const tid_t*)Ident1 - *(const tid_t*)Ident2;
}

void IPC_Type_Sys_Send(const void *Ident, size_t Length, const void *Data)
{
	_SysSendMessage( *(const tid_t*)Ident, Length, Data );
}

int IPC_Type_IPCPipe_GetSize(const void *Ident)
{
	return sizeof(int);
}
int IPC_Type_IPCPipe_Compare(const void *Ident1, const void *Ident2)
{
	return *(const int*)Ident1 - *(const int*)Ident2;
}
void IPC_Type_IPCPipe_Send(const void *Ident, size_t Length, const void *Data)
{
	 int	fd = *(const int*)Ident;
	size_t rv = _SysWrite( fd, Data, Length );
	if(rv != Length) {
		_SysDebug("Message send for IPCPipe FD %i %i req, %i sent",
			fd, Length, rv);
	}
}
