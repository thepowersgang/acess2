/*
 * Acess2 Networking Test Suite (NetTest)
 * - By John Hodge (thePowersGang)
 *
 * tcpclient.c
 * - TCP Client tester
 */
#include <vfs.h>
#include <vfs_ext.h>
#include <nettest.h>

void NetTest_Suite_Netcat(const char *Address, int Port)
{
	Uint8	addr[16];
	int type = Net_ParseAddress(Address, addr);
	if( type == 0 )
		return;

	int fd = Net_OpenSocket_TCPC(type, addr, Port);
	if( fd == -1 ) {
		Log_Error("Netcat", "herpaderp tcpc");
		return ;
	}

	char	buffer[1024];
	size_t	len;
	while( (len = VFS_Read(fd, sizeof(buffer), buffer)) )
	{
		NetTest_WriteStdout(buffer, len);
	}

	VFS_Close(fd);
}
