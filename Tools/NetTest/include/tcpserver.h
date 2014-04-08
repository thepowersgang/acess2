/*
 */
#ifndef _TCPSERVER_H_
#define _TCPSERVER_H_

#include <acess.h>
#include <vfs_ext.h>

typedef struct sNetTest_TCPServer	tNetTest_TCPServer;

extern tNetTest_TCPServer	*NetTest_TCPServer_Create(int Port);
extern void	NetTest_TCPServer_Close(tNetTest_TCPServer *Srv);
extern int	NetTest_TCPServer_FillSelect(tNetTest_TCPServer *Srv, fd_set *fds);
extern void	NetTest_TCPServer_HandleSelect(tNetTest_TCPServer *Srv, const fd_set *rfds, const fd_set *wfds, const fd_set *efds);

#endif

