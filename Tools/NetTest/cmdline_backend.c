/*
 */
#define DEBUG	1
#include "cmdline.h"
#include "tcpserver.h"
#include <vfs_ext.h>
#include <threads.h>
#include <events.h>

// === PROTOTYES ===
void	Cmdline_Backend_Thread(void *unused);

// === GLOBALS ===
tThread	*gpCmdline_WorkerThread;
tNetTest_TCPServer	*gpCmdline_TCPEchoServer;

// === CODE ===
void Cmdline_Backend_StartThread(void)
{
	ASSERT(!gpCmdline_WorkerThread);
	gpCmdline_WorkerThread = Proc_SpawnWorker(Cmdline_Backend_Thread, NULL);
}

void Cmdline_Backend_Thread(void *unused)
{
	Threads_SetName("Cmdline Worker");
	for( ;; )
	{
		fd_set	rfd, wfd, efd;
		 int	max = -1;
		
		FD_ZERO(&rfd);
		FD_ZERO(&wfd);
		
		LOG("gpCmdline_TCPEchoServer = %p", gpCmdline_TCPEchoServer);
		if(gpCmdline_TCPEchoServer)
			max = MAX(max, NetTest_TCPServer_FillSelect(gpCmdline_TCPEchoServer, &rfd));
		
		//memcpy(&wfd, &rfd, sizeof(rfd));
		memcpy(&efd, &rfd, sizeof(rfd));
		
		LOG("max = %i", max);
		int rv = VFS_Select(max+1, &rfd, &wfd, &efd, NULL, THREAD_EVENT_USER1, false);
		LOG("rv = %i", rv);
		
		if(gpCmdline_TCPEchoServer)
			NetTest_TCPServer_HandleSelect(gpCmdline_TCPEchoServer, &rfd, &wfd, &efd);
	}
}

void Cmdline_Backend_StartEchoServer(int Port)
{
	if(gpCmdline_TCPEchoServer)
	{
		// Oops, two
	}
	else
	{
		gpCmdline_TCPEchoServer = NetTest_TCPServer_Create(Port);
		Log_Debug("Cmdline", "Echo Server = %p", gpCmdline_TCPEchoServer);
		Threads_PostEvent(gpCmdline_WorkerThread, THREAD_EVENT_USER1);
	}
}
