/*
 */
#ifndef _SERVER_H_
#define _SERVER_H_

#include "common.h"

typedef struct sServer {
	struct sServer	*Next;
	 int	FD;
	char	InBuf[1024+1];
	 int	ReadPos;
	char	*Nick;
	char	Name[];
} tServer;

extern void	Servers_FillSelect(int *nfds, fd_set *rfds, fd_set *efds);
extern void	Servers_HandleSelect(int nfds, const fd_set *rfds, const fd_set *efds);
extern void	Servers_CloseAll(const char *QuitMessage);

extern tServer	*Server_Connect(const char *Name, const char *AddressString, short PortNumber);
extern  int	Server_HandleIncoming(tServer *Server);

extern const char	*Server_GetNick(const tServer *Server);
extern const char	*Server_GetName(const tServer *Server);

extern void	Server_SendCommand(tServer *Server, const char *Format, ...) __attribute__((format(__printf__,2,3)));

#endif

